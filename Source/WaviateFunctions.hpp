template <typename WaviateInput, typename ReturnType>
class WavClosure {
    const WaviateInput& input;
    public:
    WavClosure(const WaviateInput& inp);
    virtual void abstractify() = 0;
}