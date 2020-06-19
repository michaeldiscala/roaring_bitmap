#include <ruby.h>
#include <roaring.h>
#include "extconf.h"

void bitmap_free(void* data) {
  roaring_bitmap_free((roaring_bitmap_t*)data);
}

static const rb_data_type_t bitmap_type = {
  .wrap_struct_name = "bitmap",
  .function = {
    .dmark = NULL,
    .dfree = bitmap_free,
    .dsize = NULL
  },
  .data = NULL,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

VALUE bitmap_alloc(VALUE self)
{
  roaring_bitmap_t *r;
  r = roaring_bitmap_create();
  return TypedData_Wrap_Struct(self, &bitmap_type, r);
}

VALUE bitmap_m_initialize(VALUE self) {
  return self;
}

VALUE bitmap_m_deserialize(VALUE self, VALUE serialized_content) {
  roaring_bitmap_t *deserialized;

  deserialized = roaring_bitmap_portable_deserialize(
    StringValuePtr(serialized_content)
  );

 return TypedData_Wrap_Struct(self, &bitmap_type, deserialized);
}

VALUE bitmap_m_add(VALUE self, VALUE index) {
  roaring_bitmap_t *r;
  TypedData_Get_Struct(self, roaring_bitmap_t, &bitmap_type, r);

  roaring_bitmap_add(r, NUM2UINT(index));
  return Qnil;
}

VALUE bitmap_m_serialize(VALUE self) {
  roaring_bitmap_t *r;
  TypedData_Get_Struct(self, roaring_bitmap_t, &bitmap_type, r);

  // Optimize the bitmap into its most efficient format (compresses serialized
  // size)
  roaring_bitmap_run_optimize(r);

  // Serialize the bitset into a heap-allocated buffer
  uint32_t size = roaring_bitmap_portable_size_in_bytes(r);
  char *serialized_bytes = malloc(size);
  roaring_bitmap_portable_serialize(r, serialized_bytes);

  // Copy the serialized data into the Ruby-VM
  VALUE ret = rb_str_new(serialized_bytes, size);

  // Release the now unnecessary buffer
  free(serialized_bytes);

  return ret;
}

VALUE bitmap_m_cardinality(VALUE self) {
  roaring_bitmap_t *r;
  TypedData_Get_Struct(self, roaring_bitmap_t, &bitmap_type, r);

  return INT2FIX(roaring_bitmap_get_cardinality(r));
}

VALUE bitmap_m_each(VALUE self) {
  roaring_bitmap_t *r;
  TypedData_Get_Struct(self, roaring_bitmap_t, &bitmap_type, r);

  rb_need_block();

  roaring_uint32_iterator_t *  i = roaring_create_iterator(r);
  while(i->has_value) {
    rb_yield(INT2FIX(i->current_value));
    roaring_advance_uint32_iterator(i);
  }
  roaring_free_uint32_iterator(i);


  return Qnil;
}

VALUE bitmap_m_contains(VALUE self, VALUE index) {
  roaring_bitmap_t *r;
  TypedData_Get_Struct(self, roaring_bitmap_t, &bitmap_type, r);

  if (roaring_bitmap_contains(r, NUM2UINT(index))) {
    return Qtrue;
  } else {
    return Qfalse;
  }
}

void Init_roaring_bitmap_ext() {
  VALUE mRoaringBitmap;

  mRoaringBitmap = rb_define_module("RoaringBitmap");

  // Create the RoaringBitmap::Bitmap class and add methods to it
  VALUE cBitmap = rb_define_class_under(mRoaringBitmap, "Bitmap", rb_cData);
	rb_define_alloc_func(cBitmap, bitmap_alloc);
	rb_define_method(cBitmap, "initialize", bitmap_m_initialize, 0);
  rb_define_method(cBitmap, "add", bitmap_m_add, 1);
  rb_define_method(cBitmap, "serialize", bitmap_m_serialize, 0);
  rb_define_method(cBitmap, "cardinality", bitmap_m_cardinality, 0);
  rb_define_method(cBitmap, "each", bitmap_m_each, 0);
  rb_define_method(cBitmap, "contains?", bitmap_m_contains, 1);
  rb_define_singleton_method(cBitmap, "deserialize", bitmap_m_deserialize, 1);

  return;
}
