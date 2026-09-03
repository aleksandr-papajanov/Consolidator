class BankManagerPointerGesture
{
    constructor(enabled, selectAt, scrollByPointer, emit)
    {
        this.enabled = enabled;
        this.selectAt = selectAt;
        this.scrollByPointer = scrollByPointer;
        this.emit = emit;
        this.pointerDown = false;
        this.clickHandled = false;
        this.pointerX = 0;
        this.pointerY = 0;
        this.shift = false;
        this.dragging = false;
        this.lastY = 0;
    }

    begin(x, y, shift)
    {
        if (!this.enabled()) return;
        this.pointerDown = true;
        this.clickHandled = false;
        this.pointerX = x;
        this.pointerY = y;
        this.shift = Number(shift) !== 0;
        this.dragging = false;
    }

    markClickHandled()
    {
        this.clickHandled = true;
    }

    move(x, y)
    {
        if (!this.pointerDown) return;
        if (!this.dragging) {
            let distance = Math.sqrt(
                Math.pow(x - this.pointerX, 2) + Math.pow(y - this.pointerY, 2));
            if (distance < 4) return;
            this.dragging = true;
            this.lastY = this.pointerY;
            this.emit("gestureBegan");
        }
        this.scrollByPointer(this.lastY - y);
        this.lastY = y;
    }

    end(x, y)
    {
        if (!this.pointerDown) return;
        if (this.dragging) this.emit("gestureEnded");
        else if (!this.clickHandled) this.selectAt(x, y, this.shift, false);
        this.reset();
    }

    cancel()
    {
        if (this.dragging) this.emit("gestureEnded");
        this.reset();
    }

    reset()
    {
        this.pointerDown = false;
        this.shift = false;
        this.clickHandled = false;
        this.dragging = false;
    }
}

module.exports = {
    BankManagerPointerGesture: BankManagerPointerGesture
};
