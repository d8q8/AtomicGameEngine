//
// Copyright (c) 2014-2015, THUNDERBEAST GAMES LLC All rights reserved
// LICENSE: Atomic Game Engine Editor and Tools EULA
// Please see LICENSE_ATOMIC_EDITOR_AND_TOOLS.md in repository root for
// license information: https://github.com/AtomicGameEngine/AtomicGameEngine
//

import EditorEvents = require("editor/EditorEvents");
import ScriptWidget = require("ui/ScriptWidget");
import DataBinding = require("./DataBinding");

// inspectors

import MaterialInspector = require("./MaterialInspector");
import ModelInspector = require("./ModelInspector");
import NodeInspector = require("./NodeInspector");

class InspectorFrame extends ScriptWidget {

    inspectingNode: Atomic.Node;

    constructor() {

        super();

        this.gravity = Atomic.UI_GRAVITY_TOP_BOTTOM;

        this.load("AtomicEditor/editor/ui/inspectorframe.tb.txt");

        var container = this.getWidget("inspectorcontainer");

        this.subscribeToEvent(EditorEvents.EditResource, (data) => this.handleEditResource(data));
        this.subscribeToEvent(EditorEvents.ActiveNodeChange, (data) => this.handleActiveNodeChange(data));
        this.subscribeToEvent("ProjectUnloaded", (data) => this.handleProjectUnloaded(data));
        this.subscribeToEvent("NodeRemoved", (ev: Atomic.NodeRemovedEvent) => this.handleNodeRemoved(ev));


    }

    handleProjectUnloaded(data) {

        this.inspectingNode = null;
        var container = this.getWidget("inspectorcontainer");
        container.deleteAllChildren();

    }


    handleEditResource(ev: EditorEvents.EditResourceEvent) {

        var path = ev.path;

        var db = ToolCore.getAssetDatabase();
        var asset = db.getAssetByPath(path);

        if (asset) {

            this.inspectAsset(asset);

        }

    }

    handleActiveNodeChange(data) {

        var node = <Atomic.Node> data.node;

        if (!node) {

            if (this.inspectingNode) {

                this.inspectingNode = null;
                var container = this.getWidget("inspectorcontainer");
                container.deleteAllChildren();

            }

            return;
        }

        this.inspectNode(node);

    }


    inspectAsset(asset: ToolCore.Asset) {

        this.inspectingNode = null;
        var container = this.getWidget("inspectorcontainer");
        container.deleteAllChildren();

        if (asset.importerTypeName == "ModelImporter") {

            var inspector = new ModelInspector();
            container.addChild(inspector);

            inspector.inspect(asset);


        }

        if (asset.importerTypeName == "MaterialImporter") {

            var cache = Atomic.getResourceCache();

            var material = <Atomic.Material> cache.getResource("Material", asset.path);

            if (!material) {
                return;
            }

            var materialInspector = new MaterialInspector();
            container.addChild(materialInspector);

            materialInspector.inspect(asset, material);

        }

    }

    handleNodeRemoved(ev: Atomic.NodeRemovedEvent) {

        if (this.inspectingNode != ev.node)
            return;

        this.inspectingNode = null;

        var container = this.getWidget("inspectorcontainer");
        container.deleteAllChildren();

    }


    inspectNode(node: Atomic.Node) {

        if (!node) return;

        var container = this.getWidget("inspectorcontainer");
        container.deleteAllChildren();

        var inspector = new NodeInspector();
        container.addChild(inspector);

        inspector.inspect(node);

        this.inspectingNode = node;

    }

}

export = InspectorFrame;
