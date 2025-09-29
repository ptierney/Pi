//
//  MecanumRemoteApp.swift
//  MecanumRemote
//
//  Created by Patrick Tierney on 8/29/25.
//

import SwiftUI


import SwiftProtobuf
import GRPCCore
import GRPCNIOTransportHTTP2TransportServices


@main
struct MecanumRemoteApp: App {
    
    var body: some Scene {
        WindowGroup {
           // ContentView()
            NavigationView {
                DriveView()
            }
        }
    }
}
