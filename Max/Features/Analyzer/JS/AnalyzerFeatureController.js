include("../../Shared/JS/Messages/MessageFactory.js");
include("FeatureMessageAdapter.js");

function AnalyzerFeatureController() {
    this.adapter = new FeatureMessageAdapter("analyzer", MessageFactory, {});
}
