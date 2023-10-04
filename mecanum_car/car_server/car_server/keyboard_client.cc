// Original file from gRPC Source: hellowworld client example


#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"

#include <grpcpp/grpcpp.h>

#include "mecanum.grpc.pb.h"

#include <ncurses.h>

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using mecanum::CarServer;
using mecanum::MoveRequest;
using mecanum::MoveReply;

using std::cout;
using std::endl;

class CarClient {
public:
  CarClient(std::shared_ptr<Channel> channel)
      : stub_(CarServer::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  bool MoveCar(int frontBack, int leftRight) {
    // Data we are sending to the server.
    MoveRequest request;
    request.set_forward_back(frontBack);
    request.set_left_right(leftRight);

    // Container for the data we expect from the server.
    MoveReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SendMovement(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.success();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

private:
  std::unique_ptr<CarServer::Stub> stub_;
};

void keyUpArrow(CarClient& cc) {
    cout << "Up" << endl;

    cc.MoveCar(100, 0);
}

void keyDownArrow(CarClient& cc) {
    cout << "Down" << endl;

    cc.MoveCar(-100, 0);
}

void keyLeftArrow(CarClient& cc) {
    cout << "Left" << endl;

    cc.MoveCar(0, 100);
}

void keyRightArrow(CarClient& cc) {
    cout << "Right" << endl;

    cc.MoveCar(0, -100);
}


int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  std::string target_str = absl::GetFlag(FLAGS_target);
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  CarClient carClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  // https://stackoverflow.com/questions/61831114/c-how-to-get-input-from-the-arrow-keys-with-linux
  int ch;

  initscr();
  raw();
  keypad(stdscr, TRUE);
  noecho();

  while ((ch = getch()) != '#') {
    switch (ch) {
    case KEY_UP:
        keyUpArrow(carClient);
      break;

    case KEY_DOWN:
        keyDownArrow(carClient);
      break;

    case KEY_LEFT:
        keyLeftArrow(carClient);
      break;

    case KEY_RIGHT:
        keyRightArrow(carClient);
      break;

    default:
      printw("%c", ch);
    }
  }
  
  refresh();
  getch();
  endwin();

  //std::string user("world");
  //bool reply = carClient.MoveCar(100, 100);

  //std::cout << "Client received: " << reply << std::endl;

  return 0;
}
