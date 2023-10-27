xml files that descibe the shape of the particles.
empty pos file for the platelets.
empty pos file for the RBCs (RBC-h000.pos)

RBC-h018.pos: is a file with 18% hematocrit, where all the RBCs are evenly divided along the domain. This file fills a domain up-to 800x800x800 LU. 

```
<blockMultiply> 1 </blockMultiply>
```
Increasing this number will multiply the number of atomic blocks that are generated. Use with care, it will break if you use unexpected values, such as, -1, 0, 1.21411, π.
Recommended value is a multiple of 2.

