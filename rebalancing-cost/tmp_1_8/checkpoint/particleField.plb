<?xml version="1.0" ?>
<Block3D>
    <General>
        <Family> HemoParticleField3D </Family>
        <Datatype> double </Datatype>
        <Descriptor> ForcedD3Q19 </Descriptor>
        <cellDim> 0 </cellDim>
        <dynamicContent> True </dynamicContent>
        <globalId> 1 </globalId>
    </General>
    <Structure>
        <BoundingBox> 0 49 0 24 0 24 </BoundingBox>
        <EnvelopeWidth> 25 </EnvelopeWidth>
        <GridLevel> 0 </GridLevel>
        <NumComponents> 4 </NumComponents>
    </Structure>
    <Data>
        <File> ./tmp_1_8//checkpoint/particleField.dat </File>
        <Component id="0"> 0 24 0 24 0 12 </Component>
        <Component id="1"> 0 24 0 24 13 24 </Component>
        <Component id="2"> 25 49 0 24 0 12 </Component>
        <Component id="3"> 25 49 0 24 13 24 </Component>
        <Offsets> 0 0 0 0 </Offsets>
    </Data>
</Block3D>