const { AnalyzerControlBinding } = require(
    "../Features/Analyzer/Bindings/AnalyzerControlBinding.js");
const { BankManagerControlBinding } = require(
    "../Features/BankManager/Bindings/BankManagerControlBinding.js");
const { DialControlBinding } = require(
    "../Shared/Controls/Dial/DialControlBinding.js");
const { ButtonControlBinding } = require(
    "../Shared/Controls/Button/ButtonControlBinding.js");
const { ToggleControlBinding } = require(
    "../Shared/Controls/Toggle/ToggleControlBinding.js");
const { MultiValueToggleControlBinding } = require(
    "../Shared/Controls/MultiValueToggle/MultiValueToggleControlBinding.js");

function installAnalyzer(host, name, controller)
{
    host.bind(name, (send) => new AnalyzerControlBinding(
        controller.analyzer,
        controller.analyzer.presenter,
        send,
        host.client.transactions
    ));
}

function createPresenterBinding(host, type, presenter, send)
{
    if (type === "dial") {
        return new DialControlBinding(presenter, send, host.client.transactions);
    }
    if (type === "multiValueToggle") {
        return new MultiValueToggleControlBinding(
            presenter, send, host.client.transactions);
    }
    if (type === "toggle") {
        return new ToggleControlBinding(
            presenter, send, host.client.transactions);
    }
    return new ButtonControlBinding(presenter, send);
}

function installUiBindings(host)
{
    let mapping = host.mapping;
    installAnalyzer(host, mapping.equalizerAnalyzer, host.equalizer);
    installAnalyzer(host, mapping.compressorDetector, host.compressor);
    installAnalyzer(host, mapping.saturatorDetector, host.saturator);
    installAnalyzer(host, mapping.inputDetector, host.input);
    host.bind(mapping.bankManager, (send) => new BankManagerControlBinding(
        host.bankManager, host.bankManagerPresenter, send));

    [
        ["input", host.input],
        ["equalizer", host.equalizer],
        ["compressor", host.compressor],
        ["saturator", host.saturator],
        ["polish", host.polish],
        ["output", host.output]
    ].forEach((entry) => {
        let prefix = entry[0];
        entry[1].presenters.forEach((name, presenter, type) => {
            let key = prefix + name.charAt(0).toUpperCase() + name.substring(1);
            host.bind(mapping[key], (send) => {
                return createPresenterBinding(host, type, presenter, send);
            });
        });
    });
}

module.exports = {
    installUiBindings: installUiBindings
};
