var fs = require("fs");
var path = require("path");
var vm = require("vm");

var root = path.resolve(__dirname, "../..");

var commonFiles = [
  "js/Clients/NativeProtocolClient.js",
  "js/Clients/TransactionClient.js",
  "js/Clients/StateClient.js",
  "js/Clients/TargetStateClient.js",
  "js/Clients/UiTarget.js",
  "js/Clients/RegistryClient.js",
  "js/Clients/ConsolidatorClient.js",
  "js/ViewModels/ObservableValue.js",
  "js/ViewModels/StateValueViewModel.js",
  "js/ViewModels/FilterViewModel.js",
  "js/ViewModels/DetectorFilterViewModel.js",
  "js/ViewModels/BankManagerViewModel.js",
  "js/ViewModels/GainViewModel.js",
  "js/ViewModels/CompressorViewModel.js",
  "js/ViewModels/SaturatorViewModel.js",
  "js/ViewModels/EqualizerViewModel.js",
  "js/ViewModels/ConsolidatorViewModel.js",
  "js/Presenters/Core/PresentationObservable.js",
  "js/Presenters/Core/PresentationBinding.js",
  "js/Presenters/Core/Normalization.js",
  "js/Presenters/Dial/DialPresentation.js",
  "js/Presenters/Dial/DialPresenter.js",
  "js/Presenters/Button/ButtonPresentation.js",
  "js/Presenters/Button/ButtonPresenter.js",
  "js/Presenters/Analyzer/AnalyzerPresentation.js",
  "js/Presenters/Analyzer/AnalyzerPresenter.js",
  "js/Controllers/AnalyzerController.js",
  "js/Controllers/FeaturePresenterSet.js",
  "js/Controllers/EqualizerController.js",
  "js/Controllers/CompressorController.js",
  "js/Controllers/SaturatorController.js",
  "js/Controllers/BankManagerContext.js",
  "js/Controllers/BankManagerController.js",
  "js/Bindings/ControlBinding.js",
  "js/Bindings/DialControlBinding.js",
  "js/Bindings/ButtonControlBinding.js",
  "js/Bindings/AnalyzerControlBinding.js",
  "js/Bindings/BankManagerControlBinding.js",
  "js/Bindings/ControlBindings.js",
  "js/ConsolidatorUiHost.js",
];

var analyzerFiles = [
  "js/Controls/Analyzer/AnalyzerViewState.js",
  "js/Controls/Analyzer/AnalyzerLayout.js",
  "js/Controls/Analyzer/AnalyzerRenderer.js",
  "js/Controls/Analyzer/AnalyzerControl.js",
  "js/PanelBindingHost.js",
];

function load(files) {
  global.include = function () {};
  files.forEach(function (file) {
    vm.runInThisContext(fs.readFileSync(path.join(root, file), "utf8"), {
      filename: file,
    });
  });
}

function loadClientEnvironment() {
  global.mgraphics = global.mgraphics || {
    init: function () {},
    redraw: function () {},
    relative_coords: 0,
    autofill: 0,
    size: [800, 400],
  };
  global.outlet = global.outlet || function () {};
  global.arrayfromargs =
    global.arrayfromargs ||
    function (values) {
      return Array.prototype.slice.call(values);
    };
  load(commonFiles);
}

function loadExternalEnvironment() {
  global.mgraphics = {
    init: function () {},
    redraw: function () {},
    relative_coords: 0,
    autofill: 0,
    size: [800, 400],
  };
  global.outlet = function () {};
  global.arrayfromargs = function (values) {
    return Array.prototype.slice.call(values);
  };
  load(commonFiles.concat(analyzerFiles));
}

module.exports = {
  loadClientEnvironment: loadClientEnvironment,
  loadExternalEnvironment: loadExternalEnvironment,
  root: root,
};
