class ObservableValue
{
        constructor(initialValue)
        {
            this.value = initialValue === undefined ? null : initialValue;
            this.listeners = [];
        }
    
        set(value)
        {
            if (this.value === value)
            {
                return;
            }
            this.value = value;
            this.notify();
        }
    
        subscribe(callback, immediate)
        {
            this.listeners.push(callback);
            if (immediate)
            {
                callback(this.value);
            }
            return () =>
            {
                this.listeners = this.listeners.filter((listener) =>
                {
                    return listener !== callback;
                });
            };
        }
    
        notify()
        {
            for (let index = 0; index < this.listeners.length; index += 1)
            {
                this.listeners[index](this.value);
            }
        }
}

module.exports = {
    ObservableValue: ObservableValue
};
