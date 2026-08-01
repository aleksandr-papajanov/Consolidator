function BankManagerLinkPresentation(manager) {
    this.manager = manager;
}

BankManagerLinkPresentation.prototype.RefreshControlSession = function() {
    var manager = this.manager;
    var activeLinkId = manager.ActiveLinkId(manager.local);
    var activeMembers = activeLinkId ? manager.LinkMemberIds(activeLinkId) : [];
    var activeBank = manager.ActiveBank(manager.local);
    var signature = (activeLinkId && activeMembers.length >= 2
        ? activeLinkId + ":" + activeMembers.join(",")
        : "unlinked") + ":" + (activeBank ? activeBank.id : 0);
    if (manager.controlLinkSession === signature) return;
    manager.controlLinkSession = signature;
    var color = activeLinkId ? manager.LinkColor(activeLinkId) : null;
    outlet(2, "link_color", activeLinkId && color ? activeLinkId : "-",
        color ? color[0] : 0, color ? color[1] : 0,
        color ? color[2] : 0, color ? color[3] : 0);
    for (var device in manager.local.processors) {
        if (!manager.local.processors.hasOwnProperty(device)) continue;
        var processor = manager.local.processors[device];
        var definitions = manager.processorRanges[device] || {};
        var group = activeLinkId ? manager.ProcessorLinkGroup(activeLinkId, device) : null;
        var memberIds = group ? Object.keys(group.members).sort() : [];
        var isLinked = activeLinkId && memberIds.length >= 2;
        for (var parameter in processor.values) {
            if (!processor.values.hasOwnProperty(parameter) || !definitions[parameter]) continue;
            var range = definitions[parameter];
            var effective = isLinked
                ? group.EffectiveRange(manager.instanceId, parameter, range)
                : range;
            outlet(2, "processor_limits", device, parameter,
                effective.minimum, effective.maximum);
        }
    }
    var isLinkedSession = activeMembers.length >= 2;
    this.PublishFilterLimits(activeLinkId, isLinkedSession);
    this.PublishFilterPreviews(activeLinkId, isLinkedSession);
    this.PublishDetectorPreviews(activeLinkId, isLinkedSession);
};

BankManagerLinkPresentation.prototype.PublishFilterLimits = function(linkId, isLinked) {
    var manager = this.manager;
    var source = manager.ActiveBank(manager.local);
    if (!source) return;
    var members = isLinked ? manager.LinkMembers(linkId) : [];
    for (var filterId in manager.filterDefinitions) {
        if (!manager.filterDefinitions.hasOwnProperty(filterId)) continue;
        var parameters = manager.filterDefinitions[filterId];
        var sourceFilter = source.filters[filterId];
        if (!sourceFilter) continue;
        for (var parameterIndex = 0; parameterIndex < parameters.length; ++parameterIndex) {
            var definition = parameters[parameterIndex];
            var sourceValue = BankManagerMath.Normalize(
                sourceFilter.values[parameterIndex], definition);
            if (!isFinite(sourceValue)) continue;
            var minimumDelta = isLinked ? -Infinity : -sourceValue;
            var maximumDelta = isLinked ? Infinity : 1 - sourceValue;
            for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
                if (members[memberIndex].instance.id === manager.instanceId) continue;
                var filter = members[memberIndex].bank.filters[filterId];
                var value = filter ? BankManagerMath.Normalize(
                    filter.values[parameterIndex], definition) : NaN;
                if (!isFinite(value)) {
                    minimumDelta = 0;
                    maximumDelta = 0;
                    break;
                }
                minimumDelta = Math.max(minimumDelta, -value);
                maximumDelta = Math.min(maximumDelta, 1 - value);
            }
            outlet(2, "filter_limits", source.id, Number(filterId), parameterIndex,
                BankManagerMath.Denormalize(Math.max(0, sourceValue + minimumDelta), definition),
                BankManagerMath.Denormalize(Math.min(1, sourceValue + maximumDelta), definition));
        }
    }
};

BankManagerLinkPresentation.prototype.PublishDetectorPreviews = function(linkId, isLinked) {
    var manager = this.manager;
    var devices = ["compressor", "saturator"];
    for (var deviceIndex = 0; deviceIndex < devices.length; ++deviceIndex) {
        var device = devices[deviceIndex];
        outlet(2, "detector_link_preview", device, "-");
        if (!isLinked || !linkId) continue;
        var group = manager.ProcessorLinkGroup(linkId, device);
        if (!group) continue;
        for (var sourceId in group.members) {
            if (!group.members.hasOwnProperty(sourceId) || sourceId === manager.instanceId) continue;
            var values = group.members[sourceId].values;
            for (var filterId = 1; filterId <= 2; ++filterId) {
                var prefix = "detector." + filterId + ".";
                var bypass = Number(values[prefix + "bypass"]);
                var gain = Number(values[prefix + "gain"]);
                var frequency = Number(values[prefix + "frequency"]);
                var q = Number(values[prefix + "q"]);
                if (!isFinite(bypass) || !isFinite(gain) ||
                    !isFinite(frequency) || !isFinite(q)) continue;
                outlet(2, "detector_link_preview", device, linkId, sourceId,
                    filterId, bypass ? 0 : 1, gain, frequency, q);
            }
        }
    }
};

BankManagerLinkPresentation.prototype.PublishFilterPreviews = function(linkId, isLinked) {
    var manager = this.manager;
    outlet(2, "eq_link_preview", "-");
    if (!isLinked || !linkId) return;
    var members = manager.LinkMembers(linkId);
    for (var memberIndex = 0; memberIndex < members.length; ++memberIndex) {
        var member = members[memberIndex];
        if (member.instance.id === manager.instanceId) continue;
        for (var filterId in member.bank.filters) {
            if (!member.bank.filters.hasOwnProperty(filterId)) continue;
            var filter = member.bank.filters[filterId];
            var parameters = manager.filterDefinitions[filterId];
            if (!parameters) continue;
            var frequency = 0;
            var gain = 0;
            var q = 0;
            for (var parameterIndex = 0; parameterIndex < parameters.length; ++parameterIndex) {
                var parameterName = parameters[parameterIndex].name;
                var value = Number(filter.values[parameterIndex]);
                if (!isFinite(value)) continue;
                if (parameterName === "freq" || parameterName === "pivot") frequency = value;
                else if (parameterName === "gain") gain = value;
                else if (parameterName === "q") q = value;
            }
            outlet(2, "eq_link_preview", linkId, member.instance.id, Number(filterId),
                filter.bypass ? 0 : 1, frequency, gain, q,
                manager.filterTypes[filterId] || "peak");
        }
    }
};
