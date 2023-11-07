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
        <NumComponents> 6 </NumComponents>
    </Structure>
    <Data>
        <File> ./tmp_1_20//checkpoint/lattice.dat </File>
        <Component id="0"> 0 49 0 24 0 16 </Component>
        <Component id="1"> 0 49 0 24 17 33 </Component>
        <Component id="2"> 0 49 0 24 34 49 </Component>
        <Component id="3"> 0 49 25 49 0 16 </Component>
        <Component id="4"> 0 49 25 49 17 33 </Component>
        <Component id="5"> 0 49 25 49 34 49 </Component>
        <Offsets> 4357544 8705872 12807632 17165176 21513504 25615264 </Offsets>
        <DynamicsDict>
            <BGK_ExternalForce_Guo> 2 </BGK_ExternalForce_Guo>
            <BounceBack> 1 </BounceBack>
        </DynamicsDict>
    </Data>
</Block3D>
