var fs = require("fs");
var path = require("path");
var vm = require("vm");
var createRequire = require("module").createRequire;

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
  "js/Presenters/Button/ButtonPresentation.js",
  "js/Presenters/Analyzer/AnalyzerPresentation.js",
  "js/Presenters/Analyzer/BiquadCalculator.js",
  "js/Presenters/BankManager/BankManagerPresentation.js",
  "js/Presenters/History/HistoryPresentation.js",
  "js/Presenters/Slider/SliderPresentation.js",
  "js/Presenters/BankManager/BankManagerPresenter.js",
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
  "js/ConsolidatorUiApplication.js",
];

var analyzerFiles = [
  "js/Controls/Analyzer/AnalyzerViewState.js",
  "js/Controls/Analyzer/AnalyzerLayout.js",
  "js/Controls/Analyzer/AnalyzerRenderer.js",
  "js/Controls/Analyzer/AnalyzerControl.js",
];

var moduleFiles = {
  "js/Clients/NativeProtocolClient.js": true,
  "js/Clients/StateClient.js": true,
  "js/Clients/TargetStateClient.js": true,
  "js/Clients/RegistryClient.js": true,
  "js/Clients/TransactionClient.js": true,
  "js/Clients/EditingActionClient.js": true,
  "js/Clients/UiTarget.js": true,
  "js/Clients/ConsolidatorClient.js": true,
  "js/Presenters/Core/PresentationObservable.js": true,
  "js/Presenters/Core/PresentationBinding.js": true,
  "js/Presenters/Core/Normalization.js": true,
  "js/ViewModels/ObservableValue.js": true,
  "js/ViewModels/StateValueViewModel.js": true,
  "js/ViewModels/GainViewModel.js": true,
  "js/ViewModels/FilterViewModel.js": true,
  "js/ViewModels/DetectorFilterViewModel.js": true,
  "js/ViewModels/CompressorViewModel.js": true,
  "js/ViewModels/SaturatorViewModel.js": true,
  "js/ViewModels/EqualizerViewModel.js": true,
  "js/ViewModels/ConsolidatorViewModel.js": true,
  "js/ViewModels/HistoryViewModel.js": true,
  "js/ViewModels/BankManagerViewModel.js": true,
  "js/Presenters/Dial/DialPresentation.js": true,
  "js/Presenters/Button/ButtonPresentation.js": true,
  "js/Presenters/Analyzer/AnalyzerPresentation.js": true,
  "js/Presenters/Analyzer/BiquadCalculator.js": true,
  "js/Presenters/BankManager/BankManagerPresentation.js": true,
  "js/Presenters/History/HistoryPresentation.js": true,
  "js/Presenters/Slider/SliderPresentation.js": true,
  "js/Presenters/Dial/DialPresenter.js": true,
  "js/Presenters/Button/ButtonPresenter.js": true,
  "js/Presenters/Analyzer/AnalyzerPresenter.js": true,
  "js/Presenters/BankManager/BankManagerPresenter.js": true,
  "js/Presenters/History/HistoryPresenter.js": true,
  "js/Presenters/Slider/SliderPresenter.js": true,
  "js/Bindings/ControlBinding.js": true,
  "js/Bindings/DialControlBinding.js": true,
  "js/Bindings/ButtonControlBinding.js": true,
  "js/Bindings/AnalyzerControlBinding.js": true,
  "js/Bindings/BankManagerControlBinding.js": true,
  "js/Bindings/HistoryPanelBinding.js": true,
  "js/Bindings/HistoryButtonBinding.js": true,
  "js/Bindings/ControlBindings.js": true,
  "js/Controllers/AnalyzerController.js": true,
  "js/Controllers/FeaturePresenterSet.js": true,
  "js/Controllers/EqualizerController.js": true,
  "js/Controllers/CompressorController.js": true,
  "js/Controllers/SaturatorController.js": true,
  "js/Controllers/GainController.js": true,
  "js/Controllers/BankManagerContext.js": true,
  "js/Controllers/BankManagerController.js": true,
  "js/Controls/Analyzer/AnalyzerViewState.js": true,
  "js/Controls/Analyzer/AnalyzerLayout.js": true,
  "js/Controls/Analyzer/AnalyzerRenderer.js": true,
  "js/ConsolidatorUiApplication.js": true,
};

function load(files) {
  files.forEach(function (file) {
    if (moduleFiles[file]) {
      Object.keys(require(path.join(root, file))).forEach(function (name) {
        global[name] = require(path.join(root, file))[name];
      });
      return;
    }
    var absolutePath = path.join(root, file);
    var entrypoint = vm.runInThisContext(
      "(function (require) {\n" +
        fs.readFileSync(absolutePath, "utf8") +
        "\n})",
      { filename: file },
    );
    entrypoint(createRequire(absolutePath));
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
  load(commonFiles.concat(analyzerFiles));
}

module.exports = {
  loadClientEnvironment: loadClientEnvironment,
  loadExternalEnvironment: loadExternalEnvironment,
  root: root,
};
