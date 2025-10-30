
ZMAP_LENGTH = 97

print("fastfix zmap[] = {\n")

for i in range( ZMAP_LENGTH ):
    val = -75 / ( i - 113 )
    print(f"FASTFIX32( {val:.4f} ), ", end="")
print("\n};")


