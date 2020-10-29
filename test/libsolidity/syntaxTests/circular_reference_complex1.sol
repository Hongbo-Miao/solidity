contract A { function foo() public { new D(); } }
contract C { function foo() public { new A(); } }
contract D is C {}
// ----
// TypeError 7813: (87-92): Circular reference found.
