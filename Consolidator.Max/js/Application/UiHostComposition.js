const { ConsolidatorClient } = require("../Shared/Clients/ConsolidatorClient.js");
const { ConsolidatorViewModel } = require(
    "./ViewModels/ConsolidatorViewModel.js");
const { BankManagerViewModel } = require(
    "../Features/BankManager/ViewModels/BankManagerViewModel.js");
const { BankManagerPresenter } = require(
    "../Features/BankManager/Presenters/BankManagerPresenter.js");
const { EqualizerController } = require(
    "../Features/Equalizer/Controllers/EqualizerController.js");
const { CompressorController } = require(
    "../Features/Compressor/Controllers/CompressorController.js");
const { SaturatorController } = require(
    "../Features/Saturator/Controllers/SaturatorController.js");
const { InputController } = require("../Features/Input/Controllers/InputController.js");
const { OutputController } = require("../Features/Output/Controllers/OutputController.js");
const { PolishController } = require("../Features/Polish/Controllers/PolishController.js");
const { BankManagerController } = require(
    "../Features/BankManager/Controllers/BankManagerController.js");
const { BankManagerContext } = require(
    "../Features/BankManager/Controllers/BankManagerContext.js");
const { ControlBindings } = require("../Shared/Bindings/ControlBindings.js");

function createUiHostComponents(host, source, sendNative)
{
    host.client = new ConsolidatorClient(source, sendNative);
    host.viewModel = new ConsolidatorViewModel(host.client.uiTarget);
    host.equalizer = new EqualizerController(host.viewModel, host.client.scope);
    host.compressor = new CompressorController(host.viewModel, host.client.scope);
    host.saturator = new SaturatorController(host.viewModel, host.client.scope);
    host.polish = new PolishController(host.viewModel, host.client.scope);
    host.input = new InputController(host.viewModel.input, host.client.scope);
    host.output = new OutputController(host.viewModel.output, host.client.scope);

    host.equalizer.analyzer.presenter.connectSpectrum(host.client.protocol);
    host.compressor.analyzer.presenter.connectSpectrum(host.client.protocol);
    host.saturator.analyzer.presenter.connectSpectrum(host.client.protocol);
    host.input.analyzer.presenter.connectSpectrum(host.client.protocol);

    host.bankManagerViewModel = new BankManagerViewModel(
        host.client.registry,
        source,
        host.client.transactions,
        host.client.scope,
        host.client.targetState
    );
    host.bankManagerPresenter = new BankManagerPresenter(
        host.bankManagerViewModel);
    host.bankManager = new BankManagerController(new BankManagerContext(
        host.bankManagerViewModel,
        host.client.state,
        host.client.uiTarget,
        source,
        host.client.protocol,
        host.client.transactions,
        undefined,
        host.client.scope
    ));
    host.bindings = new ControlBindings();
}

function destroyUiHostComponents(host)
{
    host.bindings.destroy();
    host.equalizer.destroy();
    host.compressor.destroy();
    host.saturator.destroy();
    host.polish.destroy();
    host.input.destroy();
    host.output.destroy();
    host.bankManager.destroy();
    host.bankManagerPresenter.destroy();
    host.bankManagerViewModel.destroy();
    host.viewModel.destroy();
    host.client.destroy();
}

module.exports = {
    createUiHostComponents: createUiHostComponents,
    destroyUiHostComponents: destroyUiHostComponents
};
