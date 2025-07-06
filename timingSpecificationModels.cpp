#include <queue>
#include <filesystem>
#include "models.h"
#include "graphs.h"
#include "timingSpecificationModels.h"
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TimingSpecificationModels::generateTimingSpecificationModels(Applications* app,std::vector<NameType>* topologicalOrder) {
    if (!app) {
        std::cerr << "Error: Invalid application pointer.\n";
        return;
    }

    const std::string appName = app->appName;
    std::string appFolder = "output/" + appName;
    std::filesystem::create_directories(appFolder);

    DES* timingModel = new DES();
    int stateCount = 0;
    int interCount=0;

    // Initial state representing time spent before application arrival (ϕ_i)
    std::string currentState = std::to_string(stateCount);
    std::string interimState;
    NameType lastTask = topologicalOrder->back();
    timingModel->addState(currentState, false); // Marked initial state
    timingModel->startState=currentState;
    timingModel->des[currentState]->addTransition(currentState, "t", true); // Self-loop for time passing

    // Transition for application arrival (α_app)
    std::string nextState = std::to_string(++stateCount);
    timingModel->des[currentState]->addTransition(nextState, "α_" + appName, false);
    interCount=stateCount;


    // Time transitions until relative deadline (D_i)
    for (int i = 0; i < app->relativeDeadline; ++i) {
        timingModel->addState(nextState, false);
        currentState = nextState;
        nextState = std::to_string(++stateCount);
        timingModel->des[currentState]->addTransition(nextState, "t", true);
    }

    timingModel->addState(nextState, false);
    currentState = nextState;
    nextState = std::to_string(++stateCount);


    // Time transitions until relative deadline (D_i)
    for (int i = 0; i < app->relativeDeadline; ++i) {
        timingModel->addState(nextState, false);
        currentState = nextState;
        nextState = std::to_string(++stateCount);
        interimState=std::to_string(i+interCount+1);
        timingModel->des[interimState]->addTransition(currentState,"C_" + lastTask, false);
        timingModel->des[currentState]->addTransition(nextState, "t", true);
    }

    // Time transitions until the end of the period (P_i - D_i)
    for (int i = 1; i < app->period - app->relativeDeadline; ++i) {
        timingModel->addState(nextState, false);
        currentState = nextState;
        nextState = std::to_string(++stateCount);
        timingModel->des[currentState]->addTransition(nextState, "t",true);

    }

    //Add the final marked state....
    timingModel->addState(nextState, true);
    currentState = nextState;
    std::string interState= std::to_string(interCount);
    timingModel->des[currentState]->addTransition(interState, "α_" + appName, false);

    // Save and visualize the model
    std::string filename = "TimingSpecModel"+appName;
    std::string Imagename = "TimingSpecModel"+appName;
    Graph::visualizeDES(timingModel, Imagename);
    Graph::saveDESToFile(timingModel, filename);

    delete timingModel;
}


