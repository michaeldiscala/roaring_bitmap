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

VALUE bitmap_m_or_many(VALUE self, VALUE wrapped_bitmap_array) {
  Check_Type(wrapped_bitmap_array, T_ARRAY);

  long n_bitmaps = RARRAY_LEN(wrapped_bitmap_array);
  roaring_bitmap_t* unwrapped_bitmap_array[n_bitmaps];

  // Extract the bitmap pointers from the VALUE objects
  for (int i = 0; i < n_bitmaps; i++) {
    TypedData_Get_Struct(
      RARRAY_PTR(wrapped_bitmap_array)[i],
      roaring_bitmap_t,
      &bitmap_type,
      unwrapped_bitmap_array[i]
    );
  }

  roaring_bitmap_t *unioned = roaring_bitmap_or_many(
    n_bitmaps,
    (const roaring_bitmap_t**)unwrapped_bitmap_array
  );

  return TypedData_Wrap_Struct(self, &bitmap_type, unioned);
}

VALUE bitmap_m_or(VALUE self, VALUE other) {
  roaring_bitmap_t *unwrapped_self;
  TypedData_Get_Struct(self, roaring_bitmap_t, &bitmap_type, unwrapped_self);

  roaring_bitmap_t *unwrapped_other;
  TypedData_Get_Struct(other, roaring_bitmap_t, &bitmap_type, unwrapped_other);

  roaring_bitmap_t *result = roaring_bitmap_or(unwrapped_self, unwrapped_other);

  VALUE mRoaringBitmap = rb_define_module("RoaringBitmap");
  VALUE cBitmap = rb_define_class_under(mRoaringBitmap, "Bitmap", rb_cData);

  return TypedData_Wrap_Struct(cBitmap, &bitmap_type, result);
}

// TODO:: handle the case where 0 or 1 bitsets are passed in
// TODO:: make sure that the intersection code isn't leaking copies
VALUE bitmap_m_and_many(VALUE self, VALUE wrapped_bitmap_array) {
  Check_Type(wrapped_bitmap_array, T_ARRAY);

  long n_bitmaps = RARRAY_LEN(wrapped_bitmap_array);
  roaring_bitmap_t* unwrapped_bitmap_array[n_bitmaps];

  // Extract the bitmap pointers from the VALUE objects
  for (int i = 0; i < n_bitmaps; i++) {
    TypedData_Get_Struct(
      RARRAY_PTR(wrapped_bitmap_array)[i],
      roaring_bitmap_t,
      &bitmap_type,
      unwrapped_bitmap_array[i]
    );
  }

  if (n_bitmaps < 2)  {
    return Qnil;
  } else {
    roaring_bitmap_t *intersection = roaring_bitmap_copy(unwrapped_bitmap_array[0]);
    for (int i = 1; i < n_bitmaps; i++) {
      roaring_bitmap_and_inplace(
          intersection,
          unwrapped_bitmap_array[1]
      );
    }

    return TypedData_Wrap_Struct(self, &bitmap_type, intersection);
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
  rb_define_method(cBitmap, "or", bitmap_m_or, 1);
  rb_define_singleton_method(cBitmap, "deserialize", bitmap_m_deserialize, 1);
  rb_define_singleton_method(cBitmap, "or_many", bitmap_m_or_many, 1);
  rb_define_singleton_method(cBitmap, "and_many", bitmap_m_and_many, 1);

  return;
}
