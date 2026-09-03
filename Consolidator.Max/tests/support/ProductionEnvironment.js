var fs = require("fs");
var path = require("path");
var vm = require("vm");
var createRequire = require("module").createRequire;

var root = path.resolve(__dirname, "../..");

var commonFiles = [
  "js/Shared/Clients/NativeProtocolClient.js",
  "js/Shared/Clients/TransactionClient.js",
  "js/Shared/Clients/StateClient.js",
  "js/Shared/Clients/TargetStateClient.js",
  "js/Shared/Clients/UiTarget.js",
  "js/Shared/Clients/RegistryClient.js",
  "js/Shared/Clients/ConsolidatorClient.js",
  "js/Shared/ViewModels/ObservableValue.js",
  "js/Shared/ViewModels/StateValueViewModel.js",
  "js/Shared/ViewModels/FilterViewModel.js",
  "js/Shared/ViewModels/FilterCatalog.js",
  "js/Shared/ViewModels/DetectorFilterViewModel.js",
  "js/Features/BankManager/ViewModels/BankManagerViewModel.js",
  "js/Features/Input/ViewModels/InputViewModel.js",
  "js/Features/Output/ViewModels/OutputViewModel.js",
  "js/Features/Compressor/ViewModels/CompressorViewModel.js",
  "js/Features/Saturator/ViewModels/SaturatorViewModel.js",
  "js/Features/Equalizer/ViewModels/EqualizerViewModel.js",
  "js/Application/ViewModels/ConsolidatorViewModel.js",
  "js/Shared/Presenters/PresentationObservable.js",
  "js/Shared/Presenters/PresentationBinding.js",
  "js/Shared/Presenters/Normalization.js",
  "js/Shared/Controls/Dial/DialPresentation.js",
  "js/Shared/Controls/Button/ButtonPresentation.js",
  "js/Features/Analyzer/Presenters/AnalyzerPresentation.js",
  "js/Features/Analyzer/Presenters/BiquadCalculator.js",
  "js/Features/BankManager/Presenters/BankManagerPresentation.js",
  "js/Shared/Controls/Slider/SliderPresentation.js",
  "js/Features/BankManager/Presenters/BankManagerPresenter.js",
  "js/Shared/Controls/Dial/DialPresentation.js",
  "js/Shared/Controls/Dial/DialPresenter.js",
  "js/Shared/Controls/Button/ButtonPresentation.js",
  "js/Shared/Controls/Button/ButtonPresenter.js",
  "js/Features/Analyzer/Presenters/AnalyzerPresentation.js",
  "js/Features/Analyzer/Presenters/AnalyzerPresenter.js",
  "js/Features/Analyzer/Controllers/AnalyzerController.js",
  "js/Shared/Controllers/FeaturePresenterSet.js",
  "js/Features/Equalizer/Controllers/EqualizerController.js",
  "js/Features/Compressor/Controllers/CompressorController.js",
  "js/Features/Saturator/Controllers/SaturatorController.js",
  "js/Features/BankManager/Controllers/BankManagerContext.js",
  "js/Features/BankManager/Controllers/BankManagerController.js",
  "js/Shared/Bindings/ControlBinding.js",
  "js/Shared/Controls/Dial/DialControlBinding.js",
  "js/Shared/Controls/Button/ButtonControlBinding.js",
  "js/Features/Analyzer/Bindings/AnalyzerControlBinding.js",
  "js/Features/BankManager/Bindings/BankManagerControlBinding.js",
  "js/Shared/Bindings/ControlBindings.js",
  "js/Application/ConsolidatorUiApplication.js",
];

var analyzerFiles = [
  "js/Features/Analyzer/Controls/AnalyzerViewState.js",
  "js/Features/Analyzer/Controls/AnalyzerLayout.js",
  "js/Features/Analyzer/Controls/AnalyzerRenderer.js",
  "js/Features/Analyzer/Controls/AnalyzerControl.js",
];

var moduleFiles = {
  "js/Shared/Clients/NativeProtocolClient.js": true,
  "js/Shared/Clients/StateClient.js": true,
  "js/Shared/Clients/TargetStateClient.js": true,
  "js/Shared/Clients/RegistryClient.js": true,
  "js/Shared/Clients/TransactionClient.js": true,
  "js/Shared/Clients/UiTarget.js": true,
  "js/Shared/Clients/ConsolidatorClient.js": true,
  "js/Shared/Presenters/PresentationObservable.js": true,
  "js/Shared/Presenters/PresentationBinding.js": true,
  "js/Shared/Presenters/Normalization.js": true,
  "js/Shared/ViewModels/ObservableValue.js": true,
  "js/Shared/ViewModels/StateValueViewModel.js": true,
  "js/Features/Input/ViewModels/InputViewModel.js": true,
  "js/Features/Output/ViewModels/OutputViewModel.js": true,
  "js/Shared/ViewModels/FilterViewModel.js": true,
  "js/Shared/ViewModels/FilterCatalog.js": true,
  "js/Shared/ViewModels/DetectorFilterViewModel.js": true,
  "js/Features/Compressor/ViewModels/CompressorViewModel.js": true,
  "js/Features/Saturator/ViewModels/SaturatorViewModel.js": true,
  "js/Features/Equalizer/ViewModels/EqualizerViewModel.js": true,
  "js/Application/ViewModels/ConsolidatorViewModel.js": true,
  "js/Features/BankManager/ViewModels/BankManagerViewModel.js": true,
  "js/Shared/Controls/Dial/DialPresentation.js": true,
  "js/Shared/Controls/Button/ButtonPresentation.js": true,
  "js/Features/Analyzer/Presenters/AnalyzerPresentation.js": true,
  "js/Features/Analyzer/Presenters/BiquadCalculator.js": true,
  "js/Features/BankManager/Presenters/BankManagerPresentation.js": true,
  "js/Shared/Controls/Slider/SliderPresentation.js": true,
  "js/Shared/Controls/Dial/DialPresenter.js": true,
  "js/Shared/Controls/Button/ButtonPresenter.js": true,
  "js/Features/Analyzer/Presenters/AnalyzerPresenter.js": true,
  "js/Features/BankManager/Presenters/BankManagerPresenter.js": true,
  "js/Shared/Controls/Slider/SliderPresenter.js": true,
  "js/Shared/Bindings/ControlBinding.js": true,
  "js/Shared/Controls/Dial/DialControlBinding.js": true,
  "js/Shared/Controls/Button/ButtonControlBinding.js": true,
  "js/Features/Analyzer/Bindings/AnalyzerControlBinding.js": true,
  "js/Features/BankManager/Bindings/BankManagerControlBinding.js": true,
  "js/Shared/Bindings/ControlBindings.js": true,
  "js/Features/Analyzer/Controllers/AnalyzerController.js": true,
  "js/Shared/Controllers/FeaturePresenterSet.js": true,
  "js/Features/Equalizer/Controllers/EqualizerController.js": true,
  "js/Features/Compressor/Controllers/CompressorController.js": true,
  "js/Features/Saturator/Controllers/SaturatorController.js": true,
  "js/Features/Input/Controllers/InputController.js": true,
  "js/Features/Output/Controllers/OutputController.js": true,
  "js/Features/BankManager/Controllers/BankManagerContext.js": true,
  "js/Features/BankManager/Controllers/BankManagerController.js": true,
  "js/Features/Analyzer/Controls/AnalyzerViewState.js": true,
  "js/Features/Analyzer/Controls/AnalyzerLayout.js": true,
  "js/Features/Analyzer/Controls/AnalyzerRenderer.js": true,
  "js/Application/ConsolidatorUiApplication.js": true,
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
