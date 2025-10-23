//
// Created by ilker on 10/22/25.
//
#include "RunAction.hh"
#include "G4Run.hh"

RunAction::RunAction(): G4UserRunAction() {
}

RunAction::~RunAction() {

}

void RunAction::BeginOfRunAction(const G4Run* run) {
    startTime = chrono::high_resolution_clock::now();
    RunTime =0;
    G4cout << "### Run started ###" << G4endl;
}

void RunAction::EndOfRunAction(const G4Run* run) {
    auto duration = chrono::high_resolution_clock::now() - startTime;
    auto RunTime = chrono::duration_cast<chrono::duration<double>>(duration).count();
    std::cout << "Run time: " << RunTime << " seconds" << G4endl;
}