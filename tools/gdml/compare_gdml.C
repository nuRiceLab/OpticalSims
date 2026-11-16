// compare_gdml.C
//
// Compare two GDML files volume by volume.
//
// Usage:
//   root -l -b -q 'compare_gdml.C("official.gdml", "NoTPC.gdml")'
//   root -l -b -q 'compare_gdml.C("official.gdml", "NoTPC.gdml", "report.txt", 60)'
//   root -l -b -q 'compare_gdml.C("a.gdml", "b.gdml", "report.txt", -1)'   // list everything
//
// Arguments:
//   fileA, fileB : the two GDML files (A = reference, e.g. the official one)
//   reportFile   : optional text file for the report ("" = print only)
//   maxList      : max entries printed per section (-1 = unlimited)
//
// What is compared (all by name, using the loaded ROOT geometry):
//   1. summary numbers: volumes defined, nodes placed, materials, optical surfaces
//   2. volumes that exist only in A or only in B
//   3. volumes in both but placed a different number of times
//   4. volumes in both but with a different material, shape type or bounding box
//   5. the same as 2 and 3 grouped into "families" (digit runs replaced by '#',
//      so volArapuca_12-3 and volArapuca_97-0 both count as volArapuca_#-#),
//      which keeps the output readable for geometries with thousands of volumes
//   6. materials only in A or only in B
//   7. material definitions (density, A, Z, radiation length, mixture composition)
//   8. optical surface definitions (model, finish, type, value)
//   9. skin / border surface assignments (which volume or node pair gets which surface)
//
// The material assigned to each volume is compared in section 4.
//
// Not compared: node positions/rotations, and solid parameters beyond the
// bounding box (the exact parameters of composite solids are not easy to read back).

#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>

#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TGeoNode.h"
#include "TGeoMaterial.h"
#include "TGeoShape.h"
#include "TGeoOpticalSurface.h"
#include "TGeoBBox.h"
#include "TCollection.h"
#include "TObjArray.h"
#include "TSystem.h"

namespace cmpgdml {

struct VolInfo {
    std::string material;
    std::string shape;
    double dx = 0, dy = 0, dz = 0;
    Long64_t placed = 0;        // how many times this volume is placed in the whole tree
    Int_t nDaughters = 0;       // daughters placed directly inside this volume
};

struct MatInfo {
    double density = 0, A = 0, Z = 0, radLen = 0;
    Int_t nElem = 0;
    std::string composition;    // "Z:A:fraction;..." for mixtures
};

struct GeoData {
    std::map<std::string, MatInfo> matInfo;
    std::map<std::string, std::string> surfaces;   // name -> "model/finish/type/value"
    std::set<std::string> skinSurfaces;            // "volume -> surface"
    std::set<std::string> borderSurfaces;          // "node1 | node2 -> surface"
    std::string file;
    std::string top;
    std::map<std::string, VolInfo> vols;
    std::set<std::string> materials;
    Long64_t totalNodes = 0;
    Int_t nOptSurf = 0;
    Int_t nDuplicateNames = 0;
};

// Replace every run of digits with '#'
std::string Family(const std::string &s)
{
    std::string r;
    bool inDigits = false;
    for (char c : s) {
        if (c >= '0' && c <= '9') {
            if (!inDigits) r += '#';
            inDigits = true;
        } else {
            r += c;
            inDigits = false;
        }
    }
    return r;
}

// Post-order DFS over the volume graph (each volume visited once)
void Topo(TGeoVolume *v, std::set<TGeoVolume *> &seen, std::vector<TGeoVolume *> &post)
{
    if (!v || seen.count(v)) return;
    seen.insert(v);
    for (Int_t i = 0; i < v->GetNdaughters(); ++i)
        Topo(v->GetNode(i)->GetVolume(), seen, post);
    post.push_back(v);
}

bool Load(const char *file, GeoData &d)
{
    // TGeoManager::Import replaces the current gGeoManager
    if (TGeoManager::Import(file) == nullptr || !gGeoManager) {
        std::cerr << "ERROR: could not import " << file << std::endl;
        return false;
    }
    d.file = file;
    TGeoVolume *top = gGeoManager->GetTopVolume();
    d.top = top ? top->GetName() : "(none)";

    // Defined volumes
    TIter next(gGeoManager->GetListOfVolumes());
    TGeoVolume *v;
    while ((v = (TGeoVolume *)next())) {
        std::string name = v->GetName();
        if (d.vols.count(name)) { ++d.nDuplicateNames; continue; }
        VolInfo info;
        info.material = v->GetMaterial() ? v->GetMaterial()->GetName() : "(none)";
        TGeoShape *sh = v->GetShape();
        if (sh) {
            info.shape = sh->ClassName();
            TGeoBBox *bb = dynamic_cast<TGeoBBox *>(sh);
            if (bb) { info.dx = bb->GetDX(); info.dy = bb->GetDY(); info.dz = bb->GetDZ(); }
        } else {
            info.shape = "(none)";
        }
        info.nDaughters = v->GetNdaughters();
        d.vols[name] = info;
    }

    // Materials
    TIter nm(gGeoManager->GetListOfMaterials());
    TGeoMaterial *m;
    while ((m = (TGeoMaterial *)nm())) {
        d.materials.insert(m->GetName());
        MatInfo mi;
        mi.density = m->GetDensity();
        mi.A = m->GetA();
        mi.Z = m->GetZ();
        mi.radLen = m->GetRadLen();
        TGeoMixture *mix = dynamic_cast<TGeoMixture *>(m);
        if (mix) {
            mi.nElem = mix->GetNelements();
            std::ostringstream c;
            for (Int_t i = 0; i < mi.nElem; ++i)
                c << Form("%.6g:%.6g:%.6g;", mix->GetZmixt()[i], mix->GetAmixt()[i], mix->GetWmixt()[i]);
            mi.composition = c.str();
        }
        d.matInfo[m->GetName()] = mi;
    }

    // Optical surfaces
    if (gGeoManager->GetListOfOpticalSurfaces()) {
        d.nOptSurf = gGeoManager->GetListOfOpticalSurfaces()->GetEntries();
        TIter ns(gGeoManager->GetListOfOpticalSurfaces());
        TGeoOpticalSurface *os;
        while ((os = (TGeoOpticalSurface *)ns())) {
            d.surfaces[os->GetName()] =
                std::string(Form("model=%d finish=%d type=%d value=%.6g",
                                 (Int_t)os->GetModel(), (Int_t)os->GetFinish(),
                                 (Int_t)os->GetType(), os->GetValue()));
        }
    }

    // ---- SKIN/BORDER BLOCK: which volumes / node pairs have a surface assigned.
    // If this block does not compile on your ROOT version, comment it out.
    if (gGeoManager->GetListOfSkinSurfaces()) {
        TIter nk(gGeoManager->GetListOfSkinSurfaces());
        TGeoSkinSurface *sk;
        while ((sk = (TGeoSkinSurface *)nk())) {
            std::string vol = sk->GetVolume() ? sk->GetVolume()->GetName() : "(none)";
            std::string sf  = sk->GetSurface() ? sk->GetSurface()->GetName() : "(none)";
            d.skinSurfaces.insert(vol + " -> " + sf);
        }
    }
    if (gGeoManager->GetListOfBorderSurfaces()) {
        TIter nb(gGeoManager->GetListOfBorderSurfaces());
        TGeoBorderSurface *bs;
        while ((bs = (TGeoBorderSurface *)nb())) {
            std::string n1 = bs->GetNode1() ? bs->GetNode1()->GetName() : "(none)";
            std::string n2 = bs->GetNode2() ? bs->GetNode2()->GetName() : "(none)";
            std::string sf = bs->GetSurface() ? bs->GetSurface()->GetName() : "(none)";
            d.borderSurfaces.insert(n1 + " | " + n2 + " -> " + sf);
        }
    }
    // ---- end SKIN/BORDER BLOCK

    // Placed instances per volume: propagate multiplicities from the top down
    std::set<TGeoVolume *> seen;
    std::vector<TGeoVolume *> post;
    Topo(top, seen, post);
    std::map<TGeoVolume *, Long64_t> inst;
    if (top) inst[top] = 1;
    for (auto it = post.rbegin(); it != post.rend(); ++it) {
        TGeoVolume *p = *it;
        Long64_t n = inst[p];
        d.totalNodes += n * p->GetNdaughters();
        for (Int_t i = 0; i < p->GetNdaughters(); ++i)
            inst[p->GetNode(i)->GetVolume()] += n;
    }
    for (auto &kv : inst) {
        auto f = d.vols.find(kv.first->GetName());
        if (f != d.vols.end()) f->second.placed = kv.second;
    }
    return true;
}

bool Close(double a, double b)
{
    return std::fabs(a - b) <= 1e-6 * std::max(std::fabs(a), std::fabs(b)) + 1e-9;
}

// Print at most maxList lines of a vector of strings
void PrintList(std::ostream &out, const std::vector<std::string> &lines, Int_t maxList)
{
    Int_t n = 0;
    for (const auto &l : lines) {
        if (maxList >= 0 && n >= maxList) {
            out << "    ... and " << (lines.size() - n) << " more (use maxList=-1 to list all)\n";
            break;
        }
        out << "    " << l << "\n";
        ++n;
    }
}

struct FamInfo { Long64_t defined = 0; Long64_t placed = 0; };

std::map<std::string, FamInfo> Families(const GeoData &d)
{
    std::map<std::string, FamInfo> f;
    for (const auto &kv : d.vols) {
        FamInfo &fi = f[Family(kv.first)];
        fi.defined += 1;
        fi.placed += kv.second.placed;
    }
    return f;
}

} // namespace cmpgdml

void compare_gdml(const char *fileA, const char *fileB,
                  const char *reportFile = "", Int_t maxList = 40)
{
    using namespace cmpgdml;

    gSystem->Load("libGeom");
    gSystem->Load("libGdml");

    GeoData A, B;
    if (!Load(fileA, A)) return;
    if (!Load(fileB, B)) return;

    std::ostringstream out;
    out << "==============================================================\n";
    out << " GDML comparison\n";
    out << "   A (reference): " << A.file << "\n";
    out << "   B            : " << B.file << "\n";
    out << "==============================================================\n\n";

    // ---- 1. Summary --------------------------------------------------
    out << "1. SUMMARY\n";
    auto row = [&](const char *label, Long64_t a, Long64_t b) {
        out << "    " << label;
        for (size_t i = std::string(label).size(); i < 34; ++i) out << ' ';
        out << a << "  vs  " << b << (a == b ? "" : "   <-- differs") << "\n";
    };
    out << "    Top volume                        " << A.top << "  vs  " << B.top
        << (A.top == B.top ? "" : "   <-- differs") << "\n";
    row("Volumes defined", A.vols.size(), B.vols.size());
    row("Nodes placed (whole tree)", A.totalNodes, B.totalNodes);
    row("Materials", A.materials.size(), B.materials.size());
    row("Optical surfaces", A.nOptSurf, B.nOptSurf);
    if (A.nDuplicateNames || B.nDuplicateNames)
        out << "    (duplicate volume names skipped: A=" << A.nDuplicateNames
            << ", B=" << B.nDuplicateNames << ")\n";
    out << "\n";

    // ---- 2. Names only in one file -----------------------------------
    std::vector<std::string> onlyA, onlyB, common;
    for (const auto &kv : A.vols) (B.vols.count(kv.first) ? common : onlyA).push_back(kv.first);
    for (const auto &kv : B.vols) if (!A.vols.count(kv.first)) onlyB.push_back(kv.first);

    out << "2. VOLUMES ONLY IN A (" << onlyA.size() << ")\n";
    {
        std::vector<std::string> l;
        for (auto &n : onlyA) l.push_back(n + "   [" + A.vols[n].shape + ", " + A.vols[n].material +
                                          ", placed " + std::to_string(A.vols[n].placed) + "x]");
        PrintList(out, l, maxList);
    }
    out << "\n   VOLUMES ONLY IN B (" << onlyB.size() << ")\n";
    {
        std::vector<std::string> l;
        for (auto &n : onlyB) l.push_back(n + "   [" + B.vols[n].shape + ", " + B.vols[n].material +
                                          ", placed " + std::to_string(B.vols[n].placed) + "x]");
        PrintList(out, l, maxList);
    }
    out << "\n";

    // ---- 3. Same name, different number of placements ---------------
    out << "3. SAME NAME, DIFFERENT NUMBER OF PLACEMENTS\n";
    {
        std::vector<std::string> l;
        for (auto &n : common) {
            Long64_t a = A.vols[n].placed, b = B.vols[n].placed;
            if (a != b) l.push_back(n + "   A=" + std::to_string(a) + "  B=" + std::to_string(b));
        }
        out << "   (" << l.size() << " volumes)\n";
        PrintList(out, l, maxList);
    }
    out << "\n";

    // ---- 4. Same name, different material / shape / size -------------
    out << "4. SAME NAME, DIFFERENT MATERIAL / SHAPE / BOUNDING BOX\n";
    {
        std::vector<std::string> l;
        for (auto &n : common) {
            const VolInfo &a = A.vols[n];
            const VolInfo &b = B.vols[n];
            std::string diff;
            if (a.material != b.material) diff += " material " + a.material + " -> " + b.material + ";";
            if (a.shape != b.shape) diff += " shape " + a.shape + " -> " + b.shape + ";";
            if (!Close(a.dx, b.dx) || !Close(a.dy, b.dy) || !Close(a.dz, b.dz)) {
                std::ostringstream s;
                s << " bbox half-sizes (" << a.dx << "," << a.dy << "," << a.dz << ") -> ("
                  << b.dx << "," << b.dy << "," << b.dz << ");";
                diff += s.str();
            }
            if (a.nDaughters != b.nDaughters)
                diff += " daughters " + std::to_string(a.nDaughters) + " -> " + std::to_string(b.nDaughters) + ";";
            if (!diff.empty()) l.push_back(n + ":" + diff);
        }
        out << "   (" << l.size() << " volumes)\n";
        PrintList(out, l, maxList);
    }
    out << "\n";

    // ---- 5. Families --------------------------------------------------
    out << "5. FAMILIES (digits replaced by '#'): where defined or placed counts differ\n";
    {
        auto fa = Families(A), fb = Families(B);
        std::set<std::string> keys;
        for (auto &kv : fa) keys.insert(kv.first);
        for (auto &kv : fb) keys.insert(kv.first);
        std::vector<std::string> l;
        for (auto &k : keys) {
            FamInfo a = fa.count(k) ? fa[k] : FamInfo();
            FamInfo b = fb.count(k) ? fb[k] : FamInfo();
            if (a.defined != b.defined || a.placed != b.placed) {
                std::ostringstream s;
                s << k << "   defined A=" << a.defined << " B=" << b.defined
                  << "   placed A=" << a.placed << " B=" << b.placed;
                l.push_back(s.str());
            }
        }
        out << "   (" << l.size() << " families)\n";
        PrintList(out, l, maxList);
    }
    out << "\n";

    // ---- 6. Materials -------------------------------------------------
    out << "6. MATERIALS\n";
    {
        std::vector<std::string> la, lb;
        for (auto &m : A.materials) if (!B.materials.count(m)) la.push_back(m);
        for (auto &m : B.materials) if (!A.materials.count(m)) lb.push_back(m);
        out << "   only in A (" << la.size() << ")\n";
        PrintList(out, la, maxList);
        out << "   only in B (" << lb.size() << ")\n";
        PrintList(out, lb, maxList);
    }
    out << "\n";

    // ---- 7. Material definitions -------------------------------------
    out << "7. MATERIAL DEFINITIONS (materials present in both, with different parameters)\n";
    {
        std::vector<std::string> l;
        for (auto &kv : A.matInfo) {
            auto f = B.matInfo.find(kv.first);
            if (f == B.matInfo.end()) continue;
            const MatInfo &a = kv.second, &b = f->second;
            std::string diff;
            if (!Close(a.density, b.density)) diff += Form(" density %.6g -> %.6g;", a.density, b.density);
            if (!Close(a.A, b.A))             diff += Form(" A %.6g -> %.6g;", a.A, b.A);
            if (!Close(a.Z, b.Z))             diff += Form(" Z %.6g -> %.6g;", a.Z, b.Z);
            if (!Close(a.radLen, b.radLen))   diff += Form(" radLen %.6g -> %.6g;", a.radLen, b.radLen);
            if (a.nElem != b.nElem)           diff += Form(" nElements %d -> %d;", a.nElem, b.nElem);
            else if (a.composition != b.composition) diff += " composition differs;";
            if (!diff.empty()) l.push_back(kv.first + ":" + diff);
        }
        out << "   (" << l.size() << " materials)\n";
        PrintList(out, l, maxList);
    }
    out << "\n";

    // ---- 8. Optical surface definitions --------------------------------
    out << "8. OPTICAL SURFACE DEFINITIONS (model / finish / type / value only)\n";
    {
        std::vector<std::string> l;
        for (auto &kv : A.surfaces) {
            auto f = B.surfaces.find(kv.first);
            if (f == B.surfaces.end()) l.push_back(kv.first + ": only in A  [" + kv.second + "]");
            else if (f->second != kv.second)
                l.push_back(kv.first + ": A [" + kv.second + "]  B [" + f->second + "]");
        }
        for (auto &kv : B.surfaces)
            if (!A.surfaces.count(kv.first)) l.push_back(kv.first + ": only in B  [" + kv.second + "]");
        out << "   A has " << A.surfaces.size() << ", B has " << B.surfaces.size()
            << "  (" << l.size() << " differences)\n";
        PrintList(out, l, maxList);
    }
    out << "   NOTE: the reflectivity/efficiency tables attached to a surface are not compared here.\n\n";

    // ---- 9. Surface assignments ----------------------------------------
    out << "9. SURFACE ASSIGNMENTS (skin and border surfaces)\n";
    {
        out << "   skin surfaces:   A=" << A.skinSurfaces.size() << "  B=" << B.skinSurfaces.size() << "\n";
        out << "   border surfaces: A=" << A.borderSurfaces.size() << "  B=" << B.borderSurfaces.size() << "\n";
        std::vector<std::string> l;
        for (auto &x : A.skinSurfaces)   if (!B.skinSurfaces.count(x))   l.push_back("skin only in A:   " + x);
        for (auto &x : B.skinSurfaces)   if (!A.skinSurfaces.count(x))   l.push_back("skin only in B:   " + x);
        for (auto &x : A.borderSurfaces) if (!B.borderSurfaces.count(x)) l.push_back("border only in A: " + x);
        for (auto &x : B.borderSurfaces) if (!A.borderSurfaces.count(x)) l.push_back("border only in B: " + x);
        out << "   (" << l.size() << " differences)\n";
        PrintList(out, l, maxList);
    }
    out << "\n";

    std::cout << out.str();
    if (reportFile && std::string(reportFile).size()) {
        std::ofstream f(reportFile);
        f << out.str();
        std::cout << "Report written to " << reportFile << std::endl;
    }
}