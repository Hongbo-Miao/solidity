.. index:: ! error, revert

.. _errors:

*******************************
Errors and the Revert Statement
*******************************

Errors in Solidity provide a convenient and gas-efficient way to explain to the
user why an operation failed. They can be defined inside and outside of contracts (including interfaces and libraries).

They have to be used together with the :ref:`revert statement <revert-statement>`
which causes
all changes in the current call to be reverted and passes the error data back to the
caller.

::

    // SPDX-License-Identifier: GPL-3.0
    pragma solidity ^0.8.3;

    /// Insufficient balance for transfer. Needed `required` but only
    /// `available` available.
    /// @param available balance available.
    /// @param required requested amount to transfer.
    error InsufficientBalance(uint256 available, uint256 required);

    contract TestToken {
        mapping(address => uint) balance;
        function transfer(address to, uint256 amount) public {
            if (amount > balance[msg.sender])
                revert InsufficientBalance({
                    available: balance[msg.sender],
                    required: amount
                });
            balance[msg.sender] -= amount;
            balance[to] += amount;
        }
        // ...
    }

Errors cannot be overloaded or overridden but are inherited.
Instances of errors can only be created in ``revert`` statements.
The error creates data that is then passed to the caller with the revert operation
to either return to the off-chain component or catch it in a try/catch statement.
If you do not provide any parameters, the error only needs four bytes of
data and you can use :ref:`NatSpec <natspec>` as above
to further explain the reasons behind the error, which is not stored on chain.

More specifically, an error instance is ABI-encoded in the same way as
a function call to a function of the same name and types would be
and then used as the return data in the ``revert`` opcode.
This means that the data consists of a 4-byte selector followed by :ref:`ABI-encoded<abi>` data.
The selector is the first four bytes of the keccak256-hash of the signature of the error type.

The statement ``require(condition, "description");`` is equivalent to
``if (!condition) revert Error("description")``, assuming there is an error
``error Error(string)``, which cannot be defined in user-supplied code because
it is a built-in type.

Similarly, a failing ``assert`` or similar conditions will revert with an error
of the built-in type ``Panic(uint256)``.
