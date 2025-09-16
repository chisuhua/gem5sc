TEST(LayoutStyleTest, ManualModeRequiresCoordinates) {
    json config = R"({
        "layout": { "style": "manual" },
        "modules": [
            { "name": "cpu0", "type": "MockSim", "layout": { "x": 100, "y": 200 } }
        ]
    })"_json;

    EventQueue eq;
    ModuleFactory factory(&eq);
    factory.instantiateAll(config);

    std::ostringstream dot_output;
    // Redirect output and verify pos="100,200!" appears
    TopologyDumper::dumpToDot(factory, config, "/tmp/manual.dot");
    // Assert content...
}

TEST(GroupPlacementTest, RadialPlacementCreatesCircle) {
    json config = R"({
        "groups": {
            "ring": {
                "members": ["a", "b", "c"],
                "placement": "radial"
            }
        }
    })"_json;

    // Verify that positions are on a circle
}
