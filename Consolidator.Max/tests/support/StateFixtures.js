function makeStateFixture() {
  var paths = [];
  var batches = [];
  var batchTransactionIds = [];
  var sets = [];
  var resets = [];
  return {
    paths: paths,
    batches: batches,
    batchTransactionIds: batchTransactionIds,
    sets: sets,
    resets: resets,
    subscribe: function () {
      return function () {};
    },
    subscribeFor: function () {
      return function () {};
    },
    subscribeStatus: function () {
      return function () {};
    },
    set: function (path, value) {
      sets.push([path, value]);
    },
    setFor: function (instanceId, path, value) {
      sets.push({ instanceId: instanceId, path: path, value: value });
    },
    setMany: function (entries, callback, transactionId) {
      batches.push(entries);
      batchTransactionIds.push(transactionId);
    },
    reset: function (path) {
      resets.push(path);
    },
  };
}

module.exports = {
  makeStateFixture: makeStateFixture,
};
