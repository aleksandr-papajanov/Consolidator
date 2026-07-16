var TRACK_THICKNESS = 2;
var MARKER_SIZE = 8;
var MARKER_INNER_SIZE = 4;

function styleColor(styleName, fallback) {
    try {
        var attributeName = box.attrname_forstylemap(styleName);
        var color = box.getattr(attributeName);
        if (color && color.length >= 4) {
            return color;
        }
    }
    catch (error) {
    }
    return fallback;
}

function objectColor(attributeName, fallback) {
    try {
        var color = box.getattr(attributeName);
        if (color && color.length >= 4) {
            return color;
        }
    }
    catch (error) {
    }
    return fallback;
}

function controlIsEnabled() {
    try {
        return box.getattr("active") !== 0;
    }
    catch (error) {
        return true;
    }
}

function fillRoundedRect(x, y, width, height, radius) {
    if (width <= 0 || height <= 0) {
        return;
    }

    radius = Math.min(radius, width * 0.5, height * 0.5);
    mgraphics.rectangle(x + radius, y, width - radius * 2, height);
    mgraphics.fill();
    mgraphics.rectangle(x, y + radius, width, height - radius * 2);
    mgraphics.fill();

    mgraphics.ellipse(x, y, radius * 2, radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(x + width - radius * 2, y, radius * 2, radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(x, y + height - radius * 2, radius * 2, radius * 2);
    mgraphics.fill();
    mgraphics.ellipse(
        x + width - radius * 2,
        y + height - radius * 2,
        radius * 2,
        radius * 2
    );
    mgraphics.fill();
}

function paint() {
    var size = mgraphics.size;
    var width = size[0];
    var height = size[1];
    var value = box.getvalueof()[0];
    var enabled = controlIsEnabled();
    var track = styleColor(
        enabled ? "bordercolor" : "inactivecolor",
        [0.18, 0.18, 0.18, 1.0]
    );
    var normalActive = objectColor(
        "activeslidercolor",
        objectColor("activedialcolor", styleColor("color", [0.447, 0.035, 0.718, 1.0]))
    );
    var active = enabled
        ? normalActive
        : objectColor(
            "inactivelcdcolor",
            objectColor("inactivecolor", objectColor("textcolor", normalActive))
        );

    value = Math.max(0, Math.min(1, value));
    var left = 4;
    var right = width - 4;
    var trackY = Math.floor(height * 0.5) - TRACK_THICKNESS * 0.5;
    var trackWidth = Math.max(1, right - left);
    var handleX = left + value * trackWidth;

    mgraphics.set_source_rgba(active);
    fillRoundedRect(
        left,
        trackY,
        Math.max(0, handleX - left),
        TRACK_THICKNESS,
        TRACK_THICKNESS * 0.5
    );

    mgraphics.set_source_rgba(track);
    fillRoundedRect(
        handleX,
        trackY,
        Math.max(0, right - handleX),
        TRACK_THICKNESS,
        TRACK_THICKNESS * 0.5
    );

    mgraphics.set_source_rgba(track);
    fillRoundedRect(
        handleX - MARKER_SIZE * 0.5,
        trackY + TRACK_THICKNESS * 0.5 - MARKER_SIZE * 0.5,
        MARKER_SIZE,
        MARKER_SIZE,
        2
    );
    mgraphics.set_source_rgba(active);
    fillRoundedRect(
        handleX - MARKER_INNER_SIZE * 0.5,
        trackY + TRACK_THICKNESS * 0.5 - MARKER_INNER_SIZE * 0.5,
        MARKER_INNER_SIZE,
        MARKER_INNER_SIZE,
        1
    );

}
