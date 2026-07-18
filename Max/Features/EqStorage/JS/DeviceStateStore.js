function DeviceStateStore(commitHandler, commitContext) {
    this.dictionary = new Dict();
    this.commitHandler = commitHandler;
    this.commitContext = commitContext;
    this.dirty = false;
    this.publishScheduled = false;
    this.publishTask = new Task(this.Flush, this);
}

DeviceStateStore.PublishIntervalMs = 8;
DeviceStateStore.PersistenceDelayMs = 250;

DeviceStateStore.prototype.Attach = function(dictionaryName) {
    this.dictionary = new Dict(String(dictionaryName));
};

DeviceStateStore.prototype.Clear = function() {
    this.dictionary.clear();
};

DeviceStateStore.prototype.Get = function(path) {
    return this.dictionary.get(path);
};

DeviceStateStore.prototype.Replace = function(path, value) {
    this.dictionary.replace(path, value);
};

DeviceStateStore.prototype.SetParse = function(path, value) {
    this.dictionary.setparse(path, value);
};

DeviceStateStore.prototype.Remove = function(path) {
    this.dictionary.remove(path);
};

DeviceStateStore.prototype.Name = function() {
    return this.dictionary.name;
};

DeviceStateStore.prototype.Revision = function() {
    var revision = Number(this.Get("revision"));
    return isFinite(revision) ? Math.floor(revision) : 0;
};

DeviceStateStore.prototype.Generation = function() {
    var generation = Number(this.Get("generation"));
    return isFinite(generation) ? Math.floor(generation) : 0;
};

DeviceStateStore.prototype.CommitRevision = function() {
    this.Replace("revision", this.Revision() + 1);
};

DeviceStateStore.prototype.RequestPublish = function() {
    this.dirty = true;
    if (this.publishScheduled) return;
    this.publishScheduled = true;
    this.publishTask.schedule(DeviceStateStore.PublishIntervalMs);
};

DeviceStateStore.prototype.PublishNow = function() {
    this.dirty = true;
    if (this.publishScheduled) this.publishTask.cancel();
    this.publishScheduled = false;
    this.Flush();
};

DeviceStateStore.prototype.Flush = function() {
    this.publishScheduled = false;
    if (!this.dirty) return;
    this.dirty = false;
    var generation = this.Generation() + 1;
    this.Replace("generation", generation);
    if (this.commitHandler) {
        this.commitHandler.call(this.commitContext, this.Name(), generation);
    }
};
