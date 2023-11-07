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
        <BoundingBox> 0 49 0 49 0 49 </BoundingBox>
        <EnvelopeWidth> 2 </EnvelopeWidth>
        <GridLevel> 0 </GridLevel>
        <NumComponents> 2 </NumComponents>
    </Structure>
    <Data>
        <File> ./tmp_1_14//checkpoint/lattice.dat </File>
        <Component id="0"> 0 49 0 49 0 24 </Component>
        <Component id="1"> 0 49 0 49 25 49 </Component>
        <Offsets> 12807632 25615264 </Offsets>
        <DynamicsDict>
            <BGK_ExternalForce_Guo> 2 </BGK_ExternalForce_Guo>
            <BounceBack> 1 </BounceBack>
        </DynamicsDict>
    </Data>
</Block3D>
