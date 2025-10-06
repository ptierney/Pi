import SwiftUI
import SwiftProtobuf
import GRPCCore
import GRPCNIOTransportHTTP2
import GRPCNIOTransportHTTP2TransportServices
import Network
import NIO

@MainActor
final class DriveViewModel: ObservableObject {
    @Published var host: String = "192.168.1.139"
    @Published var port: Int = 51555
    @Published var videoPort: UInt16 = 5000
    @Published var status: String = "Disconnected"
    @Published var isConnecting: Bool = false
    @Published private(set) var isConnected: Bool = false

    // Strong ref for the permission probe (instance, not static)
    private var permissionProbe: NWConnection?

    /// Tries a lightweight TCP connect to trigger iOS "Local Network" permission.
    func triggerLocalNetworkPermissionCheck(port: Int = 22) {
        guard let nwPort = NWEndpoint.Port(rawValue: UInt16(port)) else {
            self.status = "Invalid port \(port)"
            return
        }

        let host = NWEndpoint.Host(self.host)
        let params = NWParameters.tcp
        let conn = NWConnection(host: host, port: nwPort, using: params)

        // keep a strong ref while the probe is running
        self.permissionProbe = conn

        conn.stateUpdateHandler = { [weak self] state in
            guard let self = self else { return }

            switch state {
            case .ready:
                // hop to the main actor to touch @Published state / properties
                Task { @MainActor in
                    self.status = "Ping succeeded (port \(port))"
                    self.permissionProbe = nil
                }
                conn.cancel()

            case .failed(let error):
                Task { @MainActor in
                    self.status = "Ping failed: \(error.localizedDescription)"
                    self.permissionProbe = nil
                }
                conn.cancel()

            case .waiting(let error):
                Task { @MainActor in
                    self.status = "Ping waiting: \(error.localizedDescription)"
                }

            default:
                break
            }
        }

        conn.start(queue: DispatchQueue.global(qos: .default))
    }

    func connect() {
        triggerLocalNetworkPermissionCheck(port: self.port);
    }

    func disconnect() {
        isConnected = false
        status = "Disconnected"
    }

    // MARK: RPC
    
    private func send(frontBack: Int32, leftRight: Int32, rotation: Int32 = 0) async {
        do {
            try await withGRPCClient(
                transport: .http2NIOPosix(
                    target: .ipv4(address: self.host, port: self.port),
                    transportSecurity: .plaintext
                )
            ) { client in
                var req = Mecanum_MoveRequest()
                req.forwardBack = frontBack
                req.leftRight   = leftRight
                req.rotation = rotation
                
                let client = Mecanum_CarServer.Client(wrapping: client)
                
                do {
                    let reply: Mecanum_MoveReply = try await client.sendMovement(req)
                    print("Did movement with reply success = \(reply.success)")
                } catch {
                    print("sendMovement failed: \(error)")
                }
            }
        } catch {
            print("Fail");
        }
    }
    
    private func sendWithTask(frontBack: Int32, leftRight: Int32, rotation: Int32 = 0) {
        Task {
            await send(frontBack: frontBack, leftRight: leftRight, rotation: rotation);
        }
    }

    // Arrow helpers (parity with C++)
    func up() { sendWithTask(frontBack:  100, leftRight:   0) }
    func down() { sendWithTask(frontBack: -100, leftRight:   0) }
    func left() { sendWithTask(frontBack:    0, leftRight: 100) }
    func right() { sendWithTask(frontBack:    0, leftRight: -100) }
    func turnLeft()  { sendWithTask(frontBack: 0, leftRight:  0, rotation: -100)  }
    func turnRight() { sendWithTask(frontBack: 0, leftRight: 0, rotation: 100)  }
    func stop() { sendWithTask(frontBack:    0, leftRight: 0) }
}

struct DriveView: View {
    @Environment(\.scenePhase) private var scenePhase
    @StateObject private var vm = DriveViewModel()

    var body: some View {
        VStack(spacing: 20) {
            
            /*
            HStack {
                TextField("Host", text: $vm.host)
                    .textInputAutocapitalization(.never)
                    .disableAutocorrection(true)
                    .textFieldStyle(.roundedBorder)
                TextField("Port", value: $vm.port, format: .number)
                    .keyboardType(.numberPad)
                    .frame(width: 90)
                    .textFieldStyle(.roundedBorder)
                Button(vm.isConnected ? "Disconnect" : "Connect") {
                    vm.isConnected ? vm.disconnect() : vm.connect()
                }
                .buttonStyle(.borderedProminent)
                .disabled(vm.isConnecting)
            }
            .padding(.horizontal)

             */
            
            GeometryReader { geometry in
                MJPEGViewer(host: vm.host, port: vm.videoPort)
                    .aspectRatio(contentMode: .fit)
                    .scaleEffect(0.25, anchor: .topLeading) // 25% visual scale
                    .fixedSize()
                    .background(Color.black)
            }
            
            Text(vm.status)
                .font(.footnote.monospaced())
                .foregroundColor(.secondary)

            Spacer()

            VStack(spacing: 16) {
                HStack(spacing: 40) {
                    Button(action: {}) {
                        Image(systemName: "arrow.uturn.left.circle.fill").font(.system(size: 56))
                    }
                    .pressActions(onPress: { vm.turnLeft() }, onRelease: { vm.stop() })
                    Button(action: {}) {
                        Image(systemName: "arrow.up.circle.fill").font(.system(size: 56))
                    }
                    .pressActions(onPress: { vm.up() }, onRelease: { vm.stop() })
                    Button(action: {}) {
                        Image(systemName: "arrow.uturn.right.circle.fill").font(.system(size: 56))
                    }
                    .pressActions(onPress: { vm.turnRight() }, onRelease: { vm.stop() })
                }

                HStack(spacing: 40) {
                    Button(action: {}) {
                        Image(systemName: "arrow.left.circle.fill").font(.system(size: 56))
                    }
                    .pressActions(onPress: { vm.left() }, onRelease: { vm.stop() })

                    Button(action: {}) {
                        Image(systemName: "arrow.right.circle.fill").font(.system(size: 56))
                    }
                    .pressActions(onPress: { vm.right() }, onRelease: { vm.stop() })
                }

                Button(action: {}) {
                    Image(systemName: "arrow.down.circle.fill").font(.system(size: 56))
                }
                .pressActions(onPress: { vm.down() }, onRelease: { vm.stop() })
            }
            .padding(.vertical, 24)

            Spacer()
            
            VStack(spacing: 16) {
                Button { vm.stop() } label: {
                    Image(systemName: "nosign").font(.system(size: 56))
                }
            }
            .padding(.vertical, 24)
            
            

            
            
        }
//        .navigationTitle("Mecanum Remote")
        .onChange(of: scenePhase) { oldPhase, newPhase in
            switch newPhase {
            case .active:
                print("App is active")
            case .inactive, .background:
                vm.stop()
            @unknown default:
                print("Unknown scene phase")
            }
        }
    }

}


private struct PressActions: ViewModifier {
    @State private var isDown = false
    let onPress: () -> Void
    let onRelease: () -> Void

    func body(content: Content) -> some View {
        content
            .onLongPressGesture(
                minimumDuration: 0,               // fire immediately
                maximumDistance: .infinity,
                pressing: { pressing in
                    if pressing && !isDown {
                        isDown = true
                        onPress()                  // emit move once
                    } else if !pressing && isDown {
                        isDown = false
                        onRelease()                // emit stop once
                    }
                },
                perform: {}                        // no long-press action
            )
    }
}

private extension View {
    func pressActions(onPress: @escaping () -> Void,
                      onRelease: @escaping () -> Void) -> some View {
        modifier(PressActions(onPress: onPress, onRelease: onRelease))
    }
}
