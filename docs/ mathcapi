# group `mathcapi` 

Defines basic mathematical operations for higher abstractions to use.

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public void `[`multeq_i128`](#group__mathcapi_1ga61d571f58c24fb4663cc3729f3d5613e)`(uint128_t * self,const uint128_t * other)`            | Multiply two 128 unsigned bit integers. Throws exception if pointers are invalid.
`public void `[`diveq_i128`](#group__mathcapi_1gad70a23703654281749410aef05c85c74)`(uint128_t * self,const uint128_t * other)`            | Divide two 128 unsigned bit integers and throws an exception in case of invalid pointers.
`public uint64_t `[`double_add`](#group__mathcapi_1ga7372b74e4cb87455342d39bdf3724811)`(uint64_t a,uint64_t b)`            | Addition between two double.
`public uint64_t `[`double_mult`](#group__mathcapi_1ga6390d36d23bd89bcd9bcf52385ba88f2)`(uint64_t a,uint64_t b)`            | Multiplication between two double.
`public uint64_t `[`double_div`](#group__mathcapi_1gad67af06a6b0e9dbbf3be05d6bf99257c)`(uint64_t a,uint64_t b)`            | Division between two double.
`public uint32_t `[`double_lt`](#group__mathcapi_1ga68d359e670da751723bf3d78ff759614)`(uint64_t a,uint64_t b)`            | Less than comparison between two double.
`public uint32_t `[`double_eq`](#group__mathcapi_1ga8911bd5a24e0d8287d6b3d5a81500e6a)`(uint64_t a,uint64_t b)`            | Equality check between two double.
`public uint32_t `[`double_gt`](#group__mathcapi_1gab60e3f0c6651ad497f6665e032e39f6a)`(uint64_t a,uint64_t b)`            | Greater than comparison between two double.
`public uint64_t `[`double_to_i64`](#group__mathcapi_1ga1388e46084036133e57508c18590b1ed)`(uint64_t a)`            | Convert double to 64 bit unsigned integer.
`public uint64_t `[`i64_to_double`](#group__mathcapi_1gaec506d4ee77526e67ab5f2a8ef54f2b5)`(uint64_t a)`            | Convert 64 bit unsigned integer to double (interpreted as 64 bit unsigned integer)

## Members

#### `public void `[`multeq_i128`](#group__mathcapi_1ga61d571f58c24fb4663cc3729f3d5613e)`(uint128_t * self,const uint128_t * other)` 

Multiply two 128 unsigned bit integers. Throws exception if pointers are invalid.

Multiply two 128 bit unsigned integers and assign the value to the first parameter. 
#### Parameters
* `self` Pointer to the value to be multiplied. It will be replaced with the result. 

* `other` Pointer to the Value to be multiplied.

Example: 
```cpp
uint128_t self(100);
uint128_t other(100);
multeq_i128(&self, &other);
printi128(self); // Output: 10000
```

#### `public void `[`diveq_i128`](#group__mathcapi_1gad70a23703654281749410aef05c85c74)`(uint128_t * self,const uint128_t * other)` 

Divide two 128 unsigned bit integers and throws an exception in case of invalid pointers.

Divide two 128 bit unsigned integers and assign the value to the first parameter. It will throw an exception if the value of other is zero. 
#### Parameters
* `self` Pointer to numerator. It will be replaced with the result 

* `other` Pointer to denominator Example: 
```cpp
uint128_t self(100);
uint128_t other(100);
diveq_i128(&self, &other);
printi128(self); // Output: 1
```

#### `public uint64_t `[`double_add`](#group__mathcapi_1ga7372b74e4cb87455342d39bdf3724811)`(uint64_t a,uint64_t b)` 

Addition between two double.

Get the result of addition between two double interpreted as 64 bit unsigned integer This function will first reinterpret_cast both inputs to double (50 decimal digit precision), add them together, and reinterpret_cast the result back to 64 bit unsigned integer. 
#### Parameters
* `a` Value in double interpreted as 64 bit unsigned integer 

* `b` Value in double interpreted as 64 bit unsigned integer 

#### Returns
Result of addition reinterpret_cast to 64 bit unsigned integers

Example: 
```cpp
uint64_t a = double_div( i64_to_double(5), i64_to_double(10) );
uint64_t b = double_div( i64_to_double(5), i64_to_double(2) );
uint64_t res = double_add( a, b );
printd(res); // Output: 3
```

#### `public uint64_t `[`double_mult`](#group__mathcapi_1ga6390d36d23bd89bcd9bcf52385ba88f2)`(uint64_t a,uint64_t b)` 

Multiplication between two double.

Get the result of multiplication between two double interpreted as 64 bit unsigned integer This function will first reinterpret_cast both inputs to double (50 decimal digit precision), multiply them together, and reinterpret_cast the result back to 64 bit unsigned integer. 
#### Parameters
* `a` Value in double interpreted as 64 bit unsigned integer 

* `b` Value in double interpreted as 64 bit unsigned integer 

#### Returns
Result of multiplication reinterpret_cast to 64 bit unsigned integers

Example: 
```cpp
uint64_t a = double_div( i64_to_double(10), i64_to_double(10) );
uint64_t b = double_div( i64_to_double(5), i64_to_double(2) );
uint64_t res = double_mult( a, b );
printd(res); // Output: 2.5
```

#### `public uint64_t `[`double_div`](#group__mathcapi_1gad67af06a6b0e9dbbf3be05d6bf99257c)`(uint64_t a,uint64_t b)` 

Division between two double.

Get the result of division between two double interpreted as 64 bit unsigned integer This function will first reinterpret_cast both inputs to double (50 decimal digit precision), divide numerator with denominator, and reinterpret_cast the result back to 64 bit unsigned integer. Throws an error if b is zero (after it is reinterpret_cast to double) 
#### Parameters
* `a` Numerator in double interpreted as 64 bit unsigned integer 

* `b` Denominator in double interpreted as 64 bit unsigned integer 

#### Returns
Result of division reinterpret_cast to 64 bit unsigned integers

Example: 
```cpp
uint64_t a = double_div( i64_to_double(10), i64_to_double(100) );
printd(a); // Output: 0.1
```

#### `public uint32_t `[`double_lt`](#group__mathcapi_1ga68d359e670da751723bf3d78ff759614)`(uint64_t a,uint64_t b)` 

Less than comparison between two double.

Get the result of less than comparison between two double This function will first reinterpret_cast both inputs to double (50 decimal digit precision) before doing the less than comparison. 
#### Parameters
* `a` Value in double interpreted as 64 bit unsigned integer 

* `b` Value in double interpreted as 64 bit unsigned integer 

#### Returns
1 if first input is smaller than second input, 0 otherwise

Example: 
```cpp
uint64_t a = double_div( i64_to_double(10), i64_to_double(10) );
uint64_t b = double_div( i64_to_double(5), i64_to_double(2) );
uint64_t res = double_lt( a, b );
printi(res); // Output: 1
```

#### `public uint32_t `[`double_eq`](#group__mathcapi_1ga8911bd5a24e0d8287d6b3d5a81500e6a)`(uint64_t a,uint64_t b)` 

Equality check between two double.

Get the result of equality check between two double This function will first reinterpret_cast both inputs to double (50 decimal digit precision) before doing equality check. 
#### Parameters
* `a` Value in double interpreted as 64 bit unsigned integer 

* `b` Value in double interpreted as 64 bit unsigned integer 

#### Returns
1 if first input is equal to second input, 0 otherwise

Example: 
```cpp
uint64_t a = double_div( i64_to_double(10), i64_to_double(10) );
uint64_t b = double_div( i64_to_double(5), i64_to_double(2) );
uint64_t res = double_eq( a, b );
printi(res); // Output: 0
```

#### `public uint32_t `[`double_gt`](#group__mathcapi_1gab60e3f0c6651ad497f6665e032e39f6a)`(uint64_t a,uint64_t b)` 

Greater than comparison between two double.

Get the result of greater than comparison between two double This function will first reinterpret_cast both inputs to double (50 decimal digit precision) before doing the greater than comparison. 
#### Parameters
* `a` Value in double interpreted as 64 bit unsigned integer 

* `b` Value in double interpreted as 64 bit unsigned integer 

#### Returns
1 if first input is greater than second input, 0 otherwise

Example: 
```cpp
uint64_t a = double_div( i64_to_double(10), i64_to_double(10) );
uint64_t b = double_div( i64_to_double(5), i64_to_double(2) );
uint64_t res = double_gt( a, b );
printi(res); // Output: 0
```

#### `public uint64_t `[`double_to_i64`](#group__mathcapi_1ga1388e46084036133e57508c18590b1ed)`(uint64_t a)` 

Convert double to 64 bit unsigned integer.

Convert double (interpreted as 64 bit unsigned integer) to 64 bit unsigned integer. This function will first reinterpret_cast the input to double (50 decimal digit precision) then convert it to double, then reinterpret_cast it to 64 bit unsigned integer. 
#### Parameters
* `a` - value in double interpreted as 64 bit unsigned integer 

#### Returns
Result of conversion in 64 bit unsigned integer

Example: 
```cpp
uint64_t a = double_div( i64_to_double(5), i64_to_double(2) );
uint64_t res = double_to_i64( a );
printi(res); // Output: 2
```

#### `public uint64_t `[`i64_to_double`](#group__mathcapi_1gaec506d4ee77526e67ab5f2a8ef54f2b5)`(uint64_t a)` 

Convert 64 bit unsigned integer to double (interpreted as 64 bit unsigned integer)

Convert 64 bit unsigned integer to double (interpreted as 64 bit unsigned integer). This function will convert the input to double (50 decimal digit precision) then reinterpret_cast it to 64 bit unsigned integer. 
#### Parameters
* `a` - value to be converted 

#### Returns
Result of conversion in double (interpreted as 64 bit unsigned integer)

Example: 
```cpp
uint64_t res = i64_to_double( 3 );
printd(res); // Output: 3
```

