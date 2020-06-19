require "roaring_bitmap_ext"

module RoaringBitmap
  # Most methods on this class are provided through the C-API. Several helpers/
  # wrappers are provided here when they would be more cumbersome to write
  # in the C extension
  class Bitmap
    def to_a
      [].tap do |output|
        each { |x| output << x }
      end
    end
  end
end
