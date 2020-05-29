Some tutorials:
https://www.youtube.com/watch?v=ktcWOLeWP-g

https://wiki.xxiivv.com/site/orca.html
https://esolangs.org/wiki/Orca

> Operators will always follow the capitalization on it's right side -- Orca Workshop - Basics (1/3) https://www.youtube.com/watch?v=WIzI_PSBw6o&t=17m15s

Green fg: input cells
Gray fg: "locked" (no execution)

## Operators
- `A` **add**(*a* b): Outputs sum of inputs.
- `B` **subtract**(*a* b): Outputs difference of inputs.
- `C` **clock**(*rate* mod): Outputs modulo of frame.

    equivalent to

    rate = max(rate, 1)
    if mod == 0:
        value = 0
    else if ticks % rate == 0:
        value = (value + 1) % mod
    
    default mod is 10, default rate is 1
    

- `D` **delay**(*rate* mod): Bangs on modulo of frame.

    rate = max(rate, 1)
    if mod == 0
        bang = 0
    else if mod == 1
        bang = 1
    else
        bang = ticks % (rate * mod) == 0

    default mod is 10, default rate is 1

- `E` **east**: Moves eastward, or bangs.
- `F` **if**(*a* b): Bangs if inputs are equal.
- `G` **generator**(*x* *y* *len*): Writes operands with offset.

    default offset x=0, y=1

    222G34      <- will read two values to the east 
         
         34      <- and write at offset 2,2 from G


- `H` **halt**: Halts southward operand.
- `I` **increment**(*step* mod): Increments southward operand.

    .I. 
     1  <- output

- `J` **jumper**(*val*): Outputs northward operand.
- `K` **konkat**(*len*): Reads multiple variables.
    ```
    2Kab   <- read 2 variables named a and b
      12   <- output
    ```

- `L` **less**(*a* *b*): Outputs smallest of inputs.
- `M` **multiply**(*a* b): Outputs product of inputs.
- `N` **north**: Moves Northward, or bangs.
- `O` **read**(*x* *y* read): Reads operand with offset.

    origin is the eastern cell
    default x,y = 0, 0

    1.O a   <- will read (1,0) from the origin
      a     <- and write here
    
    output = grid[origin.x + x][origin.y + y]

- `P` **push**(*key* *len* val): Writes eastward operand.

    output[key] = val

    .4Pa    
      a...
    14Pa    
      .a..


- `Q` **query**(*x* *y* *len*): Reads operands with offset.
    Same as G but to the west side

- `R` **random**(*min* max): Outputs random value.
- `S` **south**: Moves southward, or bangs.
- `T` **track**(*key* *len* val): Reads eastward operand.

    output = val[key]

- `U` **uclid**(*step* max): Bangs on Euclidean rhythm.
- `V` **variable**(*write* read): Reads and writes variable.
    ```
    aV1 <- write 1 to variable a

    .Va <- read variable a
     1
    ```

- `W` **west**: Moves westward, or bangs.
- `X` **write**(*x* *y* val): Writes operand with offset.

    origin is the cell to the south
    default x,y = 0, 0
    
    grid[origin.x + x][origin.y + y] = val

- `Y` **jymper**(*val*): Outputs westward operand.
- `Z` **lerp**(*rate* target): Transitions operand to input.
- `*` **bang**: Bangs neighboring operands.
- `#` **comment**: Halts a line.

### IO

- `:` **midi**(channel octave note velocity length): Sends a MIDI note.
- `%` **mono**(channel octave note velocity length): Sends monophonic MIDI note.
- `!` **cc**(channel knob value): Sends MIDI control change.
- `?` **pb**(channel value): Sends MIDI pitch bench.
- `;` **udp**: Sends UDP message.
- `=` **osc**(*path*): Sends OSC message.
- `$` **self**: Sends [ORCA command](#Commands).