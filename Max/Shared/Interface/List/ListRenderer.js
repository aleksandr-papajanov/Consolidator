function ListRenderer() {
}

ListRenderer.prototype.VisibleRows = function(items, scrollOffset, top, bottom, rowHeight) {
    var rows = [];
    if (!items || rowHeight <= 0) return rows;
    var firstIndex = Math.max(0, Math.floor(scrollOffset / rowHeight));
    var lastIndex = Math.min(
        items.length - 1,
        Math.ceil((scrollOffset + bottom - top) / rowHeight) - 1
    );
    for (var index = firstIndex; index <= lastIndex; index++) {
        rows.push({
            index: index,
            item: items[index],
            y: top + index * rowHeight - scrollOffset
        });
    }
    return rows;
};
