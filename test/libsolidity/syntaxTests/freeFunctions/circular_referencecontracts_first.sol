// Checks that error is triggered no matter which order
contract D {
    function f() public {
        l();
    }
}
contract C {
    constructor() { new D(); }
}
function l() {
    s();
}
function s() {
    new C();
}
// ----
// TypeError 7813: (149-154): Circular reference found.
