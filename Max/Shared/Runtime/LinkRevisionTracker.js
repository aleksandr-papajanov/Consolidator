function LinkRevisionTracker() {
    this.outgoing = {};
    this.incomingUpdates = {};
    this.incomingOperations = {};
}

LinkRevisionTracker.prototype.Next = function(linkId) {
    var key = String(linkId);
    var next = (this.outgoing[key] || 0) + 1;
    this.outgoing[key] = next;
    return next;
};

LinkRevisionTracker.prototype.AcceptUpdate = function(linkId, sourceId, revision) {
    return this.Accept(this.incomingUpdates, linkId, sourceId, revision);
};

LinkRevisionTracker.prototype.AcceptOperation = function(linkId, sourceId, revision) {
    return this.Accept(this.incomingOperations, linkId, sourceId, revision);
};

LinkRevisionTracker.prototype.Accept = function(revisions, linkId, sourceId, revision) {
    if (!linkId || !sourceId || !isFinite(revision)) return false;
    var key = String(linkId) + ":" + String(sourceId);
    if (revision <= (revisions[key] || 0)) return false;
    revisions[key] = revision;
    return true;
};
