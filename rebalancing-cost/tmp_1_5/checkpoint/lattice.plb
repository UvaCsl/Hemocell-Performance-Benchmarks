<?xml version="1.0" ?>
<Block3D>
    <General>
        <Family> BlockLattice3D </Family>
        <Datatype> double </Datatype>
        <Descriptor> ForcedD3Q19 </Descriptor>
        <cellDim> 22 </cellDim>
        <dynamicContent> True </dynamicContent>
        <globalId> 0 </globalId>
    </General>
    <Structure>
        <BoundingBox> 0 49 0 24 0 24 </BoundingBox>
        <EnvelopeWidth> 2 </EnvelopeWidth>
        <GridLevel> 0 </GridLevel>
        <NumComponents> 4 </NumComponents>
    </Structure>
    <Data>
        <File> ./tmp_1_5//checkpoint/lattice.dat </File>
        <Component id="0"> 0 24 0 24 0 12 </Component>
        <Component id="1"> 0 24 0 24 13 24 </Component>
        <Component id="2"> 25 49 0 24 0 12 </Component>
        <Component id="3"> 25 49 0 24 13 24 </Component>
        <Offsets> 1669508 3210932 4880440 6421864 </Offsets>
        <DynamicsDict>
            <BGK_ExternalForce_Guo> 2 </BGK_ExternalForce_Guo>
            <BounceBack> 1 </BounceBack>
        </DynamicsDict>
    </Data>
</Block3D>
