
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include <iostream>
#include <ostream>
#include <vector>
#include <map>
#include <memory>

#include <pigpio.h>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include "mecanum.grpc.pb.h"

using std::cout;
using std::endl;
using std::vector;
using std::map;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using mecanum::CarServer;
using mecanum::MoveRequest;
using mecanum::MoveReply;


class CarServerServiceImpl final : public CarServer::Service {
    Status SendMovement(ServerContext* context, const MoveRequest* request,
                        MoveReply* reply) override {
        // do something with the data

        return Status::OK;
    }

};


void RunServer() {
    std::string server_address = "0.0.0.0:50051";

    CarServerServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;
    // Listen on the given address without any authentication mechanism.
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // Register "service" as the instance through which we'll communicate with
    // clients. In this case it corresponds to an *synchronous* service.
    builder.RegisterService(&service);
    // Finally assemble the server.
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    server->Wait();
}

// Breadcrumbs / next steps:
// * Read the gRPC C++ Tutorial: https://grpc.io/docs/languages/cpp/basics/
// * Read section on async 
// * Implement client and server between PI and desktop computer
// * Have key presses trigger car movement

// Assume where wires protrude is the front of the car

// Naming Guide:
// LF = Left Front
// RF = Right Front
// LB = Left Back
// RB = Right Back
// F = Forward
// R = Reverse

const int WHEEL_PIN_LF_F = 23;
const int WHEEL_PIN_LF_R = 24;

const int WHEEL_PIN_RF_F = 20;
const int WHEEL_PIN_RF_R = 21;

const int WHEEL_PIN_LB_F = 5;
const int WHEEL_PIN_LB_R = 6;

const int WHEEL_PIN_RB_F = 17;
const int WHEEL_PIN_RB_R = 18;


bool running = true;

// sigint code from
// https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
void interruptHandler(int s){
    cout << "Caught signal " << s << endl;
    running = false;
}

void setupSigInt() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = interruptHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
}

int main(int argc, char** argv) {
    if (gpioInitialise() < 0) {
        cout << "Failed to start GPIO" << endl;
        return -1;
    }

    setupSigInt();
    
    cout << "Started up GPIO" << endl;

    gpioSetMode(WHEEL_PIN_LF_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_LF_R, PI_OUTPUT);
    
    gpioSetMode(WHEEL_PIN_RF_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_RF_R, PI_OUTPUT);

    gpioSetMode(WHEEL_PIN_LB_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_LB_R, PI_OUTPUT);
    
    gpioSetMode(WHEEL_PIN_RB_F, PI_OUTPUT);
    gpioSetMode(WHEEL_PIN_RB_R, PI_OUTPUT);

    gpioPWM(WHEEL_PIN_LF_F, 100);
    gpioPWM(WHEEL_PIN_RF_F, 100);
    gpioPWM(WHEEL_PIN_LB_F, 100);
    gpioPWM(WHEEL_PIN_RB_F, 100);
    
    gpioPWM(WHEEL_PIN_LF_R, 0);
    gpioPWM(WHEEL_PIN_RF_R, 0);
    gpioPWM(WHEEL_PIN_LB_R, 0);
    gpioPWM(WHEEL_PIN_RB_R, 0);

    while (running) {
        //cout << gpioTick() << endl;
        usleep(1000000/4);
    }

    // Reset pins
    gpioPWM(WHEEL_PIN_LF_F, 0);
    gpioPWM(WHEEL_PIN_LF_R, 0);
    gpioPWM(WHEEL_PIN_RF_F, 0);
    gpioPWM(WHEEL_PIN_RF_R, 0);
    gpioPWM(WHEEL_PIN_LB_F, 0);
    gpioPWM(WHEEL_PIN_LB_R, 0);
    gpioPWM(WHEEL_PIN_RB_F, 0);
    gpioPWM(WHEEL_PIN_RB_R, 0);

    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
