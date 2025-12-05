//
//  MecanumRemoteApp.swift
//  MecanumRemote
//
//  Created by Patrick Tierney on 8/29/25.
//

import SwiftUI

@main
struct MecanumRemoteApp: App {
    var body: some Scene {
        WindowGroup {
            NavigationStack {
                DriveView()
            }
        }
    }
}
