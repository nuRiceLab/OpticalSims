### GDML Generation for `dune10kt_v6_refactored`

Perl scripts based on the official DUNE GDML generation tools. The scripts support different detector configurations, including wire-free geometries and the removal of TPC regions.

#### 1. Generate the initial GDML file

```bash
perl generate_dune10kt_v6_refactored.pl -o test.gdml -w=0 -k=1
```

**Options:**

- `-o`: Specifies the output filename containing the GDML fragment configuration.
- `-w=0`: Disables TPC readout wires (`-w=1` enables wires; default).
- `-k=0`: Generates the full FD-HD geometry (default).
- `-k=1`: Generates a smaller 1×2×2 APA workspace geometry.
- `-k=2`: Generates a 1×2×6 APA geometry.
- `-t=0`: Disables placement of TPC regions, producing a geometry without the individual TPC volumes.
- `-t=1`: Enables placement of TPC regions (default).

#### 2. Generate the refactored GDML file

```bash
perl make_refactored_gdml.pl -i test.gdml -o final.gdml
```

This command processes the generated GDML fragments to produce the final refactored geometry.

**Options:**

- `-i`: Input GDML fragment configuration file.
- `-o`: Output GDML geometry filename.

#### Output

- `test.gdml` — Intermediate GDML fragment configuration.
- `final.gdml` — Final refactored GDML geometry for Geant4 and Opticks.

#### Reference

Based on the official [DUNE GDML generation tools](https://github.com/DUNE/dunecore/blob/develop/dunecore/Geometry/gdml/generate_dune10kt_v6_refactored.pl).