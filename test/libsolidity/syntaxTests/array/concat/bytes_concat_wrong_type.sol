contract C {
    bytes s;
    function f(bytes calldata c) public {
        bytes memory a;
        bytes16 b;
        uint8[] memory num;
        bytes1[] memory m;
        bytes memory d = bytes.concat(a, b, c, num, s, "abc", m);
    }
}
// ----
// TypeError 8015: (213-216): Invalid type for argument in the bytes.concat function call. bytes or fixed bytes type is required, but uint8[] provided.
// TypeError 8015: (228-229): Invalid type for argument in the bytes.concat function call. bytes or fixed bytes type is required, but bytes1[] provided.
