//
// Created by ilker on 10/22/25.
//
#include "RunAction.hh"
#include "G4Run.hh"

RunAction::RunAction(): G4UserRunAction() {
}

RunAction::~RunAction() {

}

void RunAction::BeginRun(const G4Run* run) {
    auto startTime = chrono::high_resolution_clock::now();
    RunTime =0;
    G4cout << "### Run started ###" << G4endl;
}

void RunAction::EndRun(const G4Run* run) {
    RunTime = (chrono::high_resolution_clock::now() - startTime).count();
    std::cout << "Run time: " << RunTime << " seconds" << G4endl;
}