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
        <NumComponents> 8 </NumComponents>
    </Structure>
    <Data>
        <File> ./tmp_1_21//checkpoint/lattice.dat </File>
        <Component id="0"> 0 24 0 24 0 24 </Component>
        <Component id="1"> 0 24 0 24 25 49 </Component>
        <Component id="2"> 0 24 25 49 0 24 </Component>
        <Component id="3"> 0 24 25 49 25 49 </Component>
        <Component id="4"> 25 49 0 24 0 24 </Component>
        <Component id="5"> 25 49 0 24 25 49 </Component>
        <Component id="6"> 25 49 25 49 0 24 </Component>
        <Component id="7"> 25 49 25 49 25 49 </Component>
        <Offsets> 3201908 6403816 9605724 12807632 16009540 19211448 22413356 25615264 </Offsets>
        <DynamicsDict>
            <BGK_ExternalForce_Guo> 2 </BGK_ExternalForce_Guo>
            <BounceBack> 1 </BounceBack>
        </DynamicsDict>
    </Data>
</Block3D>
