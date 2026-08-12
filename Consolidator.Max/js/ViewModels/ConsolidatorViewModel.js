function ConsolidatorViewModel(client) {
    this.client = client;
    this.selectedBank = new StateValueViewModel(
        client.state,
        "selected_bank"
    );
    this.inputGain = new GainViewModel(client.state, "main_input_gain");
    this.saturator = new SaturatorViewModel(client.state);
    this.compressor = new CompressorViewModel(client.state);
    this.equalizer = new EqualizerViewModel(client.state);
    this.outputGain = new GainViewModel(client.state, "main_output_gain");
    this.analyzer = new AnalyzerViewModel(client.analysis);
    var self = this;
    this.unsubscribeSelectedBank = this.selectedBank.subscribe(function (value) {
        if (value.value !== undefined && value.value !== null) {
            self.equalizer.showBank(value.value);
        }
    }, true);
}

ConsolidatorViewModel.prototype.getInitialStateValues = function () {
    return this.inputGain.getStateValues()
        .concat(this.saturator.getStateValues())
        .concat(this.compressor.getStateValues())
        .concat(this.equalizer.getGlobalStateValues())
        .concat(this.outputGain.getStateValues());
};

ConsolidatorViewModel.prototype.fetchValues = function (values, callback) {
    var self = this;
    var paths = values.map(function (value) {
        return value.path;
    });
    var nextBatch = 0;
    var firstError = null;

    function fetchNext() {
        if (nextBatch >= paths.length) {
            if (callback) {
                callback(firstError);
            }
            return;
        }

        var batch = paths.slice(nextBatch, nextBatch + 16);
        nextBatch += batch.length;
        self.client.state.fetchMany(batch, function (response) {
            if (response.error && !firstError) {
                firstError = response;
            }
            fetchNext();
        });
    }

    fetchNext();
};

ConsolidatorViewModel.prototype.initialize = function (callback) {
    var self = this;
    var initialValues = this.getInitialStateValues();
    var initialDone = false;
    var bankDone = false;
    var firstError = null;

    function finish(error) {
        if (error && !firstError) {
            firstError = error;
        }
        if (initialDone && bankDone && callback) {
            callback(firstError);
        }
    }

    this.fetchValues(initialValues, function (error) {
        initialDone = true;
        finish(error);
    });

    this.selectedBank.fetch(function (entry, errorResponse) {
        if (errorResponse && !firstError) {
            firstError = errorResponse;
        }
        if (entry && entry.value !== undefined && entry.value !== null) {
            self.equalizer.showBank(entry.value);
        }
        self.fetchValues(self.equalizer.getCurrentBankStateValues(), function (error) {
            bankDone = true;
            finish(error);
        });
    });
};

ConsolidatorViewModel.prototype.selectBank = function (bankId) {
    this.equalizer.showBank(bankId);
    this.selectedBank.set(bankId);
    this.fetchValues(this.equalizer.getCurrentBankStateValues());
};

ConsolidatorViewModel.prototype.destroy = function () {
    if (this.unsubscribeSelectedBank) {
        this.unsubscribeSelectedBank();
        this.unsubscribeSelectedBank = null;
    }
    this.selectedBank.destroy();
    this.inputGain.destroy();
    this.saturator.destroy();
    this.compressor.destroy();
    this.equalizer.destroy();
    this.outputGain.destroy();
    this.analyzer.destroy();
};
