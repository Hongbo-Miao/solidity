// Checks that error is triggered no matter which order
function l() {
    s();
}
function s() {
	new C();
}
contract D {
	function f() public {
		l();
	}
}
contract C {
	constructor() { new D(); }
}
// ----
// TypeError 7813: (187-192): Circular reference found.
