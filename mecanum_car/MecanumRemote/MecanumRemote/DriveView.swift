import SwiftUI
import SwiftProtobuf
import GRPCCore
import GRPCNIOTransportHTTP2TransportServices

/// iOS/Darwin HTTP/2 client transport (Network.framework via NIO Transport Services)
typealias TS = GRPCNIOTransportHTTP2TransportServices.HTTP2ClientTransport.TransportServices

@MainActor
final class DriveViewModel: ObservableObject {
    @Published var host: String = "192.168.1.139"
    @Published var port: Int = 50051
    @Published var status: String = "Disconnected"
    @Published var isConnecting: Bool = false
    @Published private(set) var isConnected: Bool = false

    private var client: Mecanum_CarServer.Client<TS>?
    private var transport: TS?

    func connect() {
        Task { [weak self] in
            guard let self, !self.isConnecting else { return }
            self.isConnecting = true
            self.status = "Connecting…"

            do {
                // TransportServices (Network.framework) HTTP/2 client transport.
                // This is the v2 pattern: init with a ResolvableTarget + config.
                let t = try TS(
                    target: .ipv4(address: self.host, port: self.port),
                    transportSecurity: .plaintext,   // use .tls() for TLS
                    config: .defaults                // NOTE: no parentheses
                )


                self.transport = t
                // after you create `t: TS`
                let base = GRPCCore.GRPCClient(transport: t)   // wrap the transport
                self.client = Mecanum_CarServer.Client(wrapping: base)   // ⬅︎ initializer takes GRPCClient<TS>

                self.isConnected = true
                self.status = "Connected"
            } catch {
                self.status = "Connect failed: \(error.localizedDescription)"
                self.transport = nil
                self.client = nil
                self.isConnected = false
            }

            self.isConnecting = false
        }
    }



    func disconnect() {
        transport = nil
        client = nil
        isConnected = false
        status = "Disconnected"
    }


    // MARK: RPC

    private func send(frontBack: Int32, leftRight: Int32) {
        print("SendMovement -> forwardBack=\(frontBack), leftRight=\(leftRight)")

        guard let client else {
            status = "Not connected"
            print("Not Connected")
            return
        }
        Task {
            do {
                var req = Mecanum_MoveRequest()
                req.forwardBack = frontBack
                req.leftRight   = leftRight
                let reply = try await client.sendMovement(req)
                print("SendMovement <- success=\(reply.success)")
                status = reply.success ? "OK" : "Server replied: failure"
            } catch {
                print("SendMovement !! error=\(error)")
                status = "RPC error: \(error.localizedDescription)"
            }
        }
    }

    // Arrow helpers (parity with your C++)
    func up()    { send(frontBack:  100, leftRight:   0) }
    func down()  { send(frontBack: -100, leftRight:   0) }
    func left()  { send(frontBack:    0, leftRight: 100) }
    func right() { send(frontBack:    0, leftRight: -100) }
}

struct DriveView: View {
    @StateObject private var vm = DriveViewModel()

    var body: some View {
        VStack(spacing: 20) {
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

            Text(vm.status)
                .font(.footnote.monospaced())
                .foregroundColor(.secondary)

            Spacer()

            VStack(spacing: 16) {
                Button { vm.up() } label: {
                    Image(systemName: "arrow.up.circle.fill").font(.system(size: 56))
                }

                HStack(spacing: 40) {
                    Button { vm.left() } label: {
                        Image(systemName: "arrow.left.circle.fill").font(.system(size: 56))
                    }
                    Button { vm.right() } label: {
                        Image(systemName: "arrow.right.circle.fill").font(.system(size: 56))
                    }
                }

                Button { vm.down() } label: {
                    Image(systemName: "arrow.down.circle.fill").font(.system(size: 56))
                }
            }
            .padding(.vertical, 24)

            Spacer()
        }
        .navigationTitle("Mecanum Remote")
    }
}
