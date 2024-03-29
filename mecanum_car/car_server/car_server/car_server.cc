
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#include <iostream>
#include <ostream>
#include <vector>
#include <map>
#include <memory>
#include <thread>

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

// Breadcrumbs / next steps:
// * Read the gRPC C++ Tutorial: https://grpc.io/docs/languages/cpp/basics/
// * Read section on async 
// * Implement client and server between PI and desktop computer
// * Have key presses trigger car movement

// Interesting site on mecanum control: https://www.roboteq.com/applications/all-blogs/5-driving-mecanum-wheels-omnidirectional-robots

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

bool RUNNING = true;
std::unique_ptr<Server> GRPC_SERVER;

void setForward(int);
void setReverse(int);

// TODO: Read this Reddit thread
// https://www.reddit.com/r/FTC/comments/2sfwro/how_do_you_program_mecanum_wheels/
// There should be a generic-ish solution

// TODO:
// When changing wheel direction, you must EASE into, otherwise there appears to be an power / amp spike

class CarServerServiceImpl final : public CarServer::Service {
    Status SendMovement(ServerContext* context, const MoveRequest* request,
                        MoveReply* reply) override {
        
        cout << "Move Request || forward_back=" << request->forward_back() << " || left_right=" << request->left_right() << endl;

        int forward_back = request->forward_back();
        int left_right = request->left_right();

        int normalized_val = (forward_back / 100.0) * 255;

        if (forward_back > 0)
            setForward(normalized_val);
        else
            setReverse(normalized_val * -1);

        return Status::OK;
    }
};


// sigint code from
// https://stackoverflow.com/questions/1641182/how-can-i-catch-a-ctrl-c-event
// Note: you shoudln't call gRPC (or any non-libc functions) in this function.
// See: https://github.com/grpc/grpc/issues/24884
void interruptHandler(int s){
    cout << "Caught signal " << s << endl;
    RUNNING = false;
}

void setupSigInt() {
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = interruptHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
}

void checkThreadShutdown(void) {
    while (RUNNING)
        usleep(1000000);
        //sleep(1);
    
    cout << "Shutting down gRPC Server" << endl;
    
    //NOTE: don't push any more work onto the server after this call
    GRPC_SERVER->Shutdown();
}

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
    GRPC_SERVER = std::unique_ptr<Server>(builder.BuildAndStart());
    
    std::cout << "Server listening on " << server_address << std::endl;

    // Wait for the server to shutdown. Note that some other thread must be
    // responsible for shutting down the server for this call to ever return.
    GRPC_SERVER->Wait();
}

// range 0 - 255
void setForwardPins(int value) {
    gpioPWM(WHEEL_PIN_LF_F, value);
    gpioPWM(WHEEL_PIN_RF_F, value);
    gpioPWM(WHEEL_PIN_LB_F, value);
    gpioPWM(WHEEL_PIN_RB_F, value);
}

// range 0 - 255
void setReversePins(int value) {
    gpioPWM(WHEEL_PIN_LF_R, value);
    gpioPWM(WHEEL_PIN_RF_R, value);
    gpioPWM(WHEEL_PIN_LB_R, value);
    gpioPWM(WHEEL_PIN_RB_R, value);
}

void setForward(int value) {
    setForwardPins(value);
    setReversePins(0);
}

void setReverse(int value) {
    setForwardPins(0);
    setReversePins(value);
}

void setPinsZero() {
  gpioPWM(WHEEL_PIN_LF_F, 0);
  gpioPWM(WHEEL_PIN_LF_R, 0);
  gpioPWM(WHEEL_PIN_RF_F, 0);
  gpioPWM(WHEEL_PIN_RF_R, 0);
  gpioPWM(WHEEL_PIN_LB_F, 0);
  gpioPWM(WHEEL_PIN_LB_R, 0);
  gpioPWM(WHEEL_PIN_RB_F, 0);
  gpioPWM(WHEEL_PIN_RB_R, 0);
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

    setPinsZero();

    std::thread t(checkThreadShutdown);
    RunServer();
    t.join();
    
    setPinsZero();
 
    cout << "Shutting down GPIO" << endl;
    
    gpioTerminate();
    return 0;
}
