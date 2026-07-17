include("../../Shared/JS/Messages/MessageFactory.js");
include("FeatureMessageAdapter.js");

function RoutingFeatureController() {
    this.adapter = new FeatureMessageAdapter("routing", MessageFactory, {});
}
