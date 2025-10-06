import UIKit
import Network
import SwiftUI

final class MJPEGStreamView: UIImageView {
    private var connection: NWConnection?
    private let queue = DispatchQueue(label: "mjpeg.stream")
    private let boundary = Data("--mjbndry".utf8) // note the two dashes here
    private var buffer = Data()
    
    func start(host: String, port: UInt16) {
        stop()
        let params = NWParameters.tcp
        let conn = NWConnection(host: NWEndpoint.Host(host), port: NWEndpoint.Port(rawValue: port)!, using: params)
        self.connection = conn

        conn.stateUpdateHandler = { [weak self] state in
            if case .failed(let err) = state { print("MJPEG conn failed:", err); self?.stop() }
        }

        conn.start(queue: queue)
        receive()
    }

    func stop() {
        connection?.cancel()
        connection = nil
        buffer.removeAll(keepingCapacity: false)
    }

    private func receive() {
        connection?.receive(minimumIncompleteLength: 1, maximumLength: 64 * 1024) { [weak self] data, _, isEOF, err in
            guard let self = self else { return }
            if let data = data { self.buffer.append(data); self.drainBuffer() }
            if isEOF || err != nil { self.stop(); return }
            self.receive()
        }
    }

    // Very small, robust multipart parser for:
    // --mjbndry\r\n
    // Content-Type: image/jpeg\r\n
    // Content-Length: N\r\n
    // \r\n
    // <N bytes of JPEG>
    private func drainBuffer() {
        while let range = buffer.range(of: boundary) {
            // ensure we have the start of a part: boundary + CRLF
            guard buffer.count > range.upperBound + 2,
                  buffer[range.upperBound] == 13, buffer[range.upperBound + 1] == 10 else {
                // drop partial garbage before boundary
                buffer.removeSubrange(..<range.lowerBound)
                break
            }
            // headers start after boundary CRLF
            var cursor = range.upperBound + 2
            guard let headerEnd = buffer.range(of: Data([13,10,13,10]), options: [], in: cursor..<buffer.endIndex) else { break }
            let headerData = buffer[cursor..<headerEnd.lowerBound]
            cursor = headerEnd.upperBound

            // parse Content-Length
            var contentLength: Int?
            if let headerStr = String(data: headerData, encoding: .utf8) {
                for line in headerStr.split(separator: "\r\n") {
                    if line.lowercased().hasPrefix("content-length:") {
                        let num = line.split(separator: ":")[1].trimmingCharacters(in: .whitespaces)
                        contentLength = Int(num)
                        break
                    }
                }
            }
            guard let len = contentLength else { break }
            // ensure whole JPEG present
            guard buffer.count >= cursor + len else { break }

            let jpegData = buffer[cursor ..< cursor + len]
            // advance buffer past this part
            let endIndex = cursor + len
            buffer.removeSubrange(..<endIndex)

            if let img = UIImage(data: jpegData) {
                DispatchQueue.main.async { self.image = img }
            }
        }
    }
}

struct MJPEGViewer: UIViewRepresentable {
    let host: String
    let port: UInt16

    func makeUIView(context: Context) -> MJPEGStreamView {
        let v = MJPEGStreamView()
        v.contentMode = .scaleAspectFit
        v.start(host: host, port: port)
        return v
    }
    func updateUIView(_ uiView: MJPEGStreamView, context: Context) {}
    static func dismantleUIView(_ uiView: MJPEGStreamView, coordinator: ()) {
        uiView.stop()
    }
    
    @available(iOS 16.0, *)
    func sizeThatFits(_ proposal: ProposedViewSize, uiView: MJPEGStreamView, context: Context) -> CGSize {
        uiView.intrinsicContentSize
    }
}
