function isPresentationBinding(value) {
    return value && typeof value === "object"
        && value.source !== undefined;
}

function bindPresentation(source, transforms) {
    transforms = transforms || {};
    return {
        source: source,
        read: transforms.read,
        write: transforms.write,
        map: transforms.map
    };
}

function presentationBindingSource(binding) {
    return isPresentationBinding(binding) ? binding.source : binding;
}

function presentationBindingValue(binding, fallback) {
    var source = presentationBindingSource(binding);
    var value = source && source.value !== undefined
        ? source.value : source;
    if (value === undefined) {
        value = fallback;
    }
    if (isPresentationBinding(binding) && typeof binding.read === "function") {
        value = binding.read(value);
    }
    if (isPresentationBinding(binding) && typeof binding.map === "function") {
        value = binding.map(value);
    }
    return value;
}

function presentationBindingWrite(binding, value, transactionId) {
    var source = presentationBindingSource(binding);
    var nextValue = value;
    if (isPresentationBinding(binding) && typeof binding.write === "function") {
        nextValue = binding.write(value);
    }
    if (source && typeof source.set === "function") {
        source.set(nextValue, undefined, transactionId);
    }
}

function subscribePresentationBinding(binding, callback, unsubscribers) {
    var source = presentationBindingSource(binding);
    if (source && typeof source.subscribe === "function") {
        unsubscribers.push(source.subscribe(callback, false));
    }
}
