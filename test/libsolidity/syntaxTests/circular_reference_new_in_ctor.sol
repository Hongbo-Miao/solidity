contract C {
	constructor() { new C(); }
}
// ----
// TypeError 7813: (30-35): Circular reference found.
