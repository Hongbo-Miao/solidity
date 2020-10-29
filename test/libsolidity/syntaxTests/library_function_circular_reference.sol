library L {
    function f() internal {
        new C();
    }
}

contract D {
    function f() public {
        L.f();
    }
}
contract C {
    constructor() { new D(); }
}

// ----
// TypeError 7813: (161-166): Circular reference found.
