// gdml_visualization.C -- ROOT viewers for OpticalSims GDML files.
//
// Quick start (from the OpticalSims top directory):
//
//   root -l 'Analysis/gdml_visualization.C("GDML/Mygeometry.gdml")'
//
//   root -l
//   .L Analysis/gdml_visualization.C
//   draw_sensor_positions("GDML/Mygeometry.gdml")    // every sensor as a marker (fast, never truncated)
//   draw_geometry("GDML/Mygeometry.gdml", 90, 0)  // geometry: 90% transparent containers, opaque sensor 
//   list_sensors("GDML/Mygeometry.gdml")          // counts per sensor LV
//   visualize_gdml(...), visualize_internal(...)    
//

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TCollection.h"
#include "TColor.h"
#include "TGeoBBox.h"
#include "TGeoManager.h"
#include "TGeoMatrix.h"
#include "TGeoNode.h"
#include "TGeoVolume.h"
#include "TLegend.h"
#include "TPolyMarker3D.h"
#include "TROOT.h"
#include "TString.h"
#include "TSystem.h"
#include "TView.h"
#include "TVirtualPad.h"
#include "TVirtualViewer3D.h"
#include "TGLViewer.h"

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

// A volume counts as a sensor if its name contains one of these (case sensitive).
// Matches volPMTbarrel / volPMTcap and the DUNE arapuca/PD volumes.
static const std::vector<std::string> kSensorTags = {"PMT", "Arapuca", "PhotonDetector", "Sensor"};

static bool IsSensorVolume(const TGeoVolume* vol)
{
    if (!vol) return false;
    const std::string name = vol->GetName();
    for (const auto& tag : kSensorTags)
        if (name.find(tag) != std::string::npos) return true;
    return false;
}

static bool LoadGDML(const char* filename)
{
    gSystem->Load("libGeom");
    gSystem->Load("libGdml");
    if (TGeoManager::Import(filename) == nullptr || !gGeoManager || !gGeoManager->GetTopVolume()) {
        std::cerr << "Error: Failed to import GDML file: " << filename << std::endl;
        return false;
    }
    std::cout << "--- Successfully loaded GDML: " << filename << std::endl;
    return true;
}

// Depth of the deepest node below the top volume (top = level 0).
static Int_t MaxDepth()
{
    Int_t depth = 0;
    TGeoIterator next(gGeoManager->GetTopVolume());
    while (next()) depth = std::max(depth, next.GetLevel());
    return depth;
}

// Explicit vis level -> TGeo's automatic "max 10000 visible nodes" depth is switched off.
static void ShowAllLevels()
{
    const Int_t depth = MaxDepth();
    gGeoManager->SetVisLevel(depth + 1);
    std::cout << "--- Vis level set to " << depth + 1 << " (deepest node at level " << depth << ")" << std::endl;
}

// Set transparency on every volume that is currently visible.
// sensorTransp is used for sensor volumes, containerTransp for everything else.
void SetAllTransparency(Int_t containerTransp, Int_t sensorTransp = -1)
{
    if (!gGeoManager) return;
    if (sensorTransp < 0) sensorTransp = containerTransp;
    TIter next(gGeoManager->GetListOfVolumes());
    TGeoVolume* vol;
    while ((vol = (TGeoVolume*)next())) {
        if (!vol->IsVisible()) continue;
        vol->SetTransparency(IsSensorVolume(vol) ? sensorTransp : containerTransp);
    }
    std::cout << "--- Transparency: containers " << containerTransp << "%, sensors " << sensorTransp << "%" << std::endl;
}

// Give sensor volumes distinct colours (in the GL viewer the line colour is the fill colour).
static void ColourSensors()
{
    const Color_t palette[] = {kAzure + 1, kOrange + 1, kGreen + 2, kMagenta + 1, kRed + 1};
    Int_t i = 0;
    TIter next(gGeoManager->GetListOfVolumes());
    TGeoVolume* vol;
    while ((vol = (TGeoVolume*)next())) {
        if (!IsSensorVolume(vol)) continue;
        vol->SetLineColor(palette[i++ % 5]);
        vol->SetVisibility(kTRUE);
    }
}

// Lift the OpenGL viewer's per-frame time budget so large scenes are drawn completely.
static TGLViewer* UnlimitedGLDrawTime(Float_t ms_still = 1e6, Float_t ms_moving = 1e6)
{
    if (!gPad) return nullptr;
    TGLViewer* v = dynamic_cast<TGLViewer*>(gPad->GetViewer3D());
    if (!v) {
        std::cerr << "Warning: no TGLViewer attached to the pad (batch mode?)" << std::endl;
        return nullptr;
    }
    v->SetMaxSceneDrawTimeHQ(ms_still);   // default 5000 ms
    v->SetMaxSceneDrawTimeLQ(ms_moving);  // default 100 ms (used while rotating)
    v->UpdateScene();
    v->ResetCurrentCamera();
    v->RequestDraw();
    return v;
}

// ---------------------------------------------------------------------------
// 1) every sensor as a marker -- quick, complete check of the PMT layout
// ---------------------------------------------------------------------------
void draw_sensor_positions(const char* filename, Bool_t useGL = kTRUE)
{
    if (!LoadGDML(filename)) return;

    std::map<std::string, TPolyMarker3D*> markers;   // one marker set per sensor LV
    Double_t lo[3] = {1e30, 1e30, 1e30}, hi[3] = {-1e30, -1e30, -1e30};

    TGeoIterator next(gGeoManager->GetTopVolume());
    TGeoNode* node;
    while ((node = next())) {
        TGeoVolume* vol = node->GetVolume();
        if (!IsSensorVolume(vol)) continue;
        // centre of the shape's bounding box (= dome centre for the clipped LUCiD spheres)
        const Double_t* local = ((TGeoBBox*)vol->GetShape())->GetOrigin();
        Double_t global[3];
        next.GetCurrentMatrix()->LocalToMaster(local, global);
        auto& pm = markers[vol->GetName()];
        if (!pm) pm = new TPolyMarker3D();
        pm->SetNextPoint(global[0], global[1], global[2]);
        for (int k = 0; k < 3; ++k) { lo[k] = std::min(lo[k], global[k]); hi[k] = std::max(hi[k], global[k]); }
    }
    if (markers.empty()) {
        std::cerr << "No sensor volumes found (tags: PMT, Arapuca, PhotonDetector, Sensor)" << std::endl;
        return;
    }

    TCanvas* c = new TCanvas("cPMT", Form("Sensors in %s", filename), 900, 900);
    TView* view = TView::CreateView(1);
    Double_t half = 0;
    for (int k = 0; k < 3; ++k) half = std::max(half, 0.55 * (hi[k] - lo[k]));
    view->SetRange(-half, -half, -half, half, half, half);
    view->ShowAxis();

    const Color_t palette[] = {kAzure + 1, kOrange + 1, kGreen + 2, kMagenta + 1, kRed + 1};
    TLegend* leg = new TLegend(0.02, 0.90, 0.40, 0.99);
    Int_t i = 0, total = 0;
    for (auto& kv : markers) {
        kv.second->SetMarkerStyle(20);
        kv.second->SetMarkerSize(0.35);
        kv.second->SetMarkerColor(palette[i++ % 5]);
        kv.second->Draw();
        leg->AddEntry(kv.second, Form("%s: %d", kv.first.c_str(), kv.second->GetN()), "p");
        total += kv.second->GetN();
        std::cout << "  " << kv.first << ": " << kv.second->GetN() << std::endl;
    }
    leg->SetHeader(Form("%d sensors", total));
    leg->Draw();
    std::cout << "--- Drew " << total << " sensors" << std::endl;
    c->Update();

    if (useGL) {
        c->GetViewer3D("ogl");   // open the same markers in the OpenGL viewer (rotate/zoom)
        UnlimitedGLDrawTime();
    }
}

// ---------------------------------------------------------------------------
// 2) full geometry in the OpenGL viewer
// ---------------------------------------------------------------------------
//   containerTransp : transparency (0-100) of non-sensor volumes that stay visible
//   sensorTransp    : transparency of sensor volumes (0 = opaque, fastest, clearest)
//   hideContainers  : hide the world and its first-level daughters (tank / cryostat),
//                     their daughters are still drawn
//   nSegments       : polygons per circle when tessellating spheres/tubes (ROOT default 20)
void draw_geometry(const char* filename, Int_t containerTransp = 90, Int_t sensorTransp = 0,
                   Bool_t hideContainers = kTRUE, Int_t nSegments = 16)
{
    if (!LoadGDML(filename)) return;
    gGeoManager->SetNsegments(nSegments);
    ShowAllLevels();

    TGeoVolume* topVol = gGeoManager->GetTopVolume();
    std::cout << "Top volume '" << topVol->GetName() << "' has " << topVol->GetNdaughters()
              << " daughter nodes." << std::endl;

    if (hideContainers) {
        topVol->SetVisibility(kFALSE);
        for (Int_t i = 0; i < topVol->GetNdaughters(); ++i) {
            TGeoVolume* v = topVol->GetNode(i)->GetVolume();
            if (IsSensorVolume(v)) continue;
            v->SetVisibility(kFALSE);           // hides the container only, not its daughters
            v->SetVisDaughters(kTRUE);
            std::cout << "Hiding container: " << v->GetName() << " (" << v->GetNdaughters() << " daughters)" << std::endl;
        }
    }
    ColourSensors();
    SetAllTransparency(containerTransp, sensorTransp);

    topVol->Draw("ogl");
    UnlimitedGLDrawTime();
}

// ---------------------------------------------------------------------------
// 3) text summary: how many placements of each sensor LV
// ---------------------------------------------------------------------------
void list_sensors(const char* filename)
{
    if (!LoadGDML(filename)) return;
    std::map<std::string, Int_t> count;
    TGeoIterator next(gGeoManager->GetTopVolume());
    TGeoNode* node;
    Int_t all = 0;
    while ((node = next())) {
        ++all;
        if (IsSensorVolume(node->GetVolume())) ++count[node->GetVolume()->GetName()];
    }
    Int_t total = 0;
    for (auto& kv : count) { std::cout << "  " << kv.first << ": " << kv.second << std::endl; total += kv.second; }
    std::cout << "--- " << total << " sensor placements out of " << all << " nodes, max depth " << MaxDepth() << std::endl;
}

// ---------------------------------------------------------------------------
// original entry points, kept with the bugs fixed
// ---------------------------------------------------------------------------
void visualize_gdml(const char* filename, Int_t transparency = 70)
{
    // was: vis level 5, hide first-level volumes, everything `transparency`% transparent,
    //      default GL time budget  -> PMTs now opaque and draw time unlimited
    draw_geometry(filename, transparency, 0, kTRUE);
}

void visualize_internal(const char* gdml_filename, Int_t transparency = 70)
{
    // was: no vis level (auto depth -> 1 for >10000 nodes, i.e. no PMTs)
    draw_geometry(gdml_filename, transparency, transparency, kTRUE);
}

// Default entry point: `root -l 'Analysis/gdml_visualization.C("file.gdml")'`
void gdml_visualization(const char* filename = "GDML/SK_LUCiD_opticks.gdml")
{
    list_sensors(filename);
    draw_geometry(filename);
}
