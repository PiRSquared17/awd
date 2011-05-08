#include <stdlib.h>
#include <cstdio>

#include "mesh.h"
#include "block.h"
#include "name.h"
#include "attr.h"
#include "util.h"

#ifdef WIN32
#include "awdw32.h"
#else
#include <unistd.h>
#endif



AWDSubMesh::AWDSubMesh()
{
    this->first_stream = NULL;
    this->last_stream = NULL;
    this->next = NULL;
}

void 
AWDSubMesh::add_stream(AWD_mesh_str_type type, AWD_str_ptr data, awd_uint32 num_elements)
{
    AWDMeshDataStream *str;

    str = new AWDMeshDataStream((awd_uint8)type, data, num_elements);

    if (this->first_stream == NULL) {
        this->first_stream = str;
    }
    else {
        this->last_stream->next = str;
    }

    this->last_stream = str;
    this->last_stream->next = NULL;
}






AWDMeshData::AWDMeshData(const char *name, awd_uint16 name_len) :
    AWDBlock(MESH_DATA),
    AWDNamedElement(name, name_len),
    AWDAttrElement() 
{
    this->first_sub = NULL;
    this->last_sub = NULL;
    this->skeleton = NULL;
    this->bind_mtx = NULL;
    this->num_subs = 0;
}

void 
AWDMeshData::add_sub_mesh(AWDSubMesh *sub)
{
    if (this->first_sub == NULL) {
        this->first_sub = sub;
    }
    else {
        this->last_sub->next = sub;
    }
    
    this->num_subs++;
    this->last_sub = sub;
}


int
AWDMeshData::get_num_subs()
{
    return this->num_subs;
}


AWDSubMesh *
AWDMeshData::get_sub_at(int idx)
{
    if (idx < this->num_subs) {
        int cur_idx;
        AWDSubMesh *cur;

        cur_idx = 0;
        cur = this->first_sub;
        while (cur) {
            if (cur_idx == idx)
                return cur;

            cur_idx++;
            cur = cur->next;
        }
    }

    return NULL;
}


AWDSkeleton *
AWDMeshData::get_skeleton()
{
    return this->skeleton;
}


void
AWDMeshData::set_skeleton(AWDSkeleton *skeleton)
{
    this->skeleton = skeleton;
}


awd_float64 *
AWDMeshData::get_bind_mtx()
{
    return this->bind_mtx;
}


void
AWDMeshData::set_bind_mtx(awd_float64 *bind_mtx)
{
    this->bind_mtx = bind_mtx;
}


awd_uint32
AWDMeshData::calc_body_length(awd_bool wide)
{
    AWDSubMesh *sub;
    awd_uint32 mesh_len;

    // Calculate length of entire mesh 
    // data (not block header)
    mesh_len = sizeof(awd_uint16); // Num subs
    mesh_len += sizeof(awd_uint16) + this->get_name_length();
    mesh_len += this->calc_attr_length(true,true);
    sub = this->first_sub;
    while (sub) {
        AWDDataStream *str;
        
        // add size of mat ID and sub-mesh
        // length (both awd_uint32)
        mesh_len += 8;

        str = sub->first_stream;
        while (str) {
            mesh_len += 5 + str->get_length(wide);

            str = str->next;
        }

        sub = sub->next;
    }


    return mesh_len;
}


void
AWDMeshData::prepare_write()
{

    // Set skeleton addr property if there is 
    // a skeleton bound to this mesh.
    if (this->skeleton) {
        AWD_attr_val_ptr skel_val;
        AWD_attr_val_ptr bind_val;
        skel_val.addr = (awd_baddr *)malloc(sizeof(awd_baddr));
        *skel_val.addr = this->skeleton->get_addr();
        this->properties->set(PROP_MD_SKELETON, skel_val, sizeof(awd_baddr), AWD_ATTR_BADDR);

        if (this->bind_mtx == NULL)
            this->bind_mtx = awdutil_id_mtx4(NULL);

        bind_val.f64 = this->bind_mtx;
        this->properties->set(PROP_MD_BIND_MTX, bind_val, 16*sizeof(awd_float64), AWD_ATTR_MTX4);
    }
}


void
AWDMeshData::write_body(int fd, awd_bool wide)
{
    awd_uint16 num_subs_be;
    AWDSubMesh *sub;

    // Write name and sub count
    num_subs_be = UI16(this->num_subs);
    awdutil_write_varstr(fd, this->get_name(), this->get_name_length()); 
    write(fd, &num_subs_be, sizeof(awd_uint16));

    // Write list of optional properties
    this->properties->write_attributes(fd);

    // Write all sub-meshes
    sub = this->first_sub;
    while (sub) {
        AWDDataStream *str;
        awd_baddr mat_id;
        awd_uint32 sub_len;

        sub_len = 0;
        str = sub->first_stream;
        while (str) {
            sub_len += (str->get_length(wide) + 5);

            str = str->next;
        }

        // TODO: Find material correctly
        //mat_id = _awd_get_block_id_by_data(sub->material);
        mat_id = 0;

        // Verify byte-order
        mat_id = UI32(mat_id);
        sub_len = UI32(sub_len);

        // Write sub-mesh header
        write(fd, &mat_id, sizeof(awd_baddr)); 
        write(fd, &sub_len, sizeof(awd_uint32));

        str = sub->first_stream;
        while(str) {
            str->write_stream(fd, wide);

            str = str->next;
        }

        sub = sub->next;
    }
    
    // Write list of user attributes
    this->user_attributes->write_attributes(fd);
}





AWDMeshInst::AWDMeshInst(const char *name, awd_uint16 name_len, AWDMeshData *data) :
    AWDBlock(MESH_INSTANCE),
    AWDNamedElement(name, name_len),
    AWDAttrElement()
{
    awd_float64 *mtx;

    mtx = awdutil_id_mtx4(NULL);

    this->set_data(data);
    this->set_transform(mtx);
}


AWDMeshInst::AWDMeshInst(const char *name, awd_uint16 name_len, AWDMeshData *data, awd_float64 *mtx) :
    AWDBlock(MESH_INSTANCE),
    AWDNamedElement(name, name_len),
    AWDAttrElement() 
{
    this->set_data(data);
    this->set_transform(mtx);
}


AWDMeshData *
AWDMeshInst::get_data()
{
    return this->data;
}


void
AWDMeshInst::set_data(AWDMeshData *data)
{
    this->data = data;
}


void
AWDMeshInst::set_transform(awd_float64 *mtx)
{
    this->transform_mtx = mtx;
}


awd_uint32
AWDMeshInst::calc_body_length(awd_bool wide)
{
    return 136 + sizeof(awd_uint16) + this->get_name_length();
}



void
AWDMeshInst::write_body(int fd, awd_bool wide)
{
    awd_baddr parent_addr;
    awd_baddr data_addr;

    // TODO: Use (and create) awd->scene_blocks instead
    // Get IDs for references, verify byte-order
    parent_addr = 0;
    data_addr = UI32(this->data->get_addr());

    // Write scene block common fields
    // TODO: Move this to separate base class
    write(fd, &parent_addr, sizeof(awd_baddr));
    awdutil_write_mtx4(fd, this->transform_mtx);
    awdutil_write_varstr(fd, this->get_name(), this->get_name_length());

    // Write mesh data address
    write(fd, &data_addr, sizeof(awd_uint32));
}
