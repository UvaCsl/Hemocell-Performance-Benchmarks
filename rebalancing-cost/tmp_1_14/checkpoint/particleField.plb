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
        <BoundingBox> 0 49 0 49 0 49 </BoundingBox>
        <EnvelopeWidth> 25 </EnvelopeWidth>
        <GridLevel> 0 </GridLevel>
        <NumComponents> 2 </NumComponents>
    </Structure>
    <Data>
        <File> ./tmp_1_14//checkpoint/particleField.dat </File>
        <Component id="0"> 0 49 0 49 0 24 </Component>
        <Component id="1"> 0 49 0 49 25 49 </Component>
        <Offsets> 0 0 </Offsets>
    </Data>
</Block3D>
