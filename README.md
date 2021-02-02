

# Ral

```
MODULE := ITEM*

ITEM := fn IDENT ( IDENT : TYPE ,* ) -> 

E := IDENT
   | LITERAL
   | E ∡ E
   | ()

S := S₁ ; S₂
   | { S* }
   | from E₁ do S until E₂
   | let IDENT := E
   | unlet IDENT := E
   | let IDENT := IDENT ( E,* )
   | unlet IDENT := ~IDENT ( E,* )
   | do S₁ yield S₂ undo
   | if E₁ S₁ else S₂ thus E₂
   | IDENT <> IDENT
   | IDENT ∘= E


TYPE := int
      | i32
      | void

```

Hereby, `∘` is one of `+`, `-`, `*`, and `/` and `∡` is one of `<`, `<=`, `=`, `!=`, `>=`, and `>`.


# Semantics

The loop `from E₁ do S until E₂` evaluates `E₁` first.
If this is 1, it runs `S`, then `E₂`. If `E₂` is 0, we run `S` and `E₂` again, otherwise the loop stops.



