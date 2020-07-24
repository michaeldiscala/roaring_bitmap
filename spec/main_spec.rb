require "roaring_bitmap_ext"


# TODO:: in most of the tests for combining bitsets there should be assertions
# that the original bitsets are not modified at all.
RSpec.describe RoaringBitmap::Bitmap do
  let(:sample_values) { [5, 500, 33, 17, 313] }
  let(:sample_bitset) do
    bitset = described_class.new
    sample_values.each { |value| bitset.add(value) }
    bitset
  end

  describe "#to_a" do
    subject { sample_bitset.to_a.sort }

    it "returns the set bits" do
      expect(subject).to eq(sample_values.sort)
    end

    context "when the same value is set multiple times" do
      let(:sample_values) { [2, 2] }

      it "returns the value only once" do
        expect(subject).to eq([2])
      end
    end
  end

  describe "#contains?" do
    context "when the bit is in the set" do
      subject { sample_bitset.contains?(sample_values.first) }
      it { is_expected.to eq(true) }
    end

    context "when the bit is not in the set" do
      let(:sample_values) { [2] }
      subject { sample_bitset.contains?(33) }
      it { is_expected.to eq(false) }
    end
  end

  describe "#cardinality" do
    subject { sample_bitset.cardinality }
    it { is_expected.to eq(sample_values.uniq.count) }

    context "when the same value is set multiple times" do
      let(:sample_values) { [2, 2, 3] }

      it { is_expected.to eq(sample_values.uniq.count) }
    end
  end

  describe "#serialize and #deserialize" do
    let(:cloned) { described_class.deserialize(sample_bitset.serialize) }

    it "contains the same values" do
      expect(sample_bitset.to_a).to eq(cloned.to_a)
    end
  end

  describe "#or" do
    let(:other_sample_values) { [6, 501, 34, 187, 313] }
    let(:other_sample_bitset) do
      bitset = described_class.new
      other_sample_values.each { |value| bitset.add(value) }
      bitset
    end

    subject { other_sample_bitset.or(sample_bitset) }

    it "returns a bitset containing all bits from the inputs" do
      expect(subject.to_a).to match_array(other_sample_values | sample_values)
    end
  end

  describe "#and" do
    let(:other_sample_values) { [6, 501, 34, 187, 313] }
    let(:other_sample_bitset) do
      bitset = described_class.new
      other_sample_values.each { |value| bitset.add(value) }
      bitset
    end

    subject { other_sample_bitset.and(sample_bitset) }

    it "returns a bitset containing all bits from the inputs" do
      expect(subject.to_a).to match_array(other_sample_values & sample_values)
    end
  end


  describe ".or_many" do
    let(:enabled_bits) do
      Array.new(500) { (rand * 10_000).to_i }
    end

    let(:sample_bitsets) do
      enabled_bits.map do |bit|
        described_class.new.tap { |bitset| bitset.add(bit) }
      end
    end

    subject { described_class.or_many(sample_bitsets) }

    it "returns a bitset" do
      expect(subject).to be_a(described_class)
    end

    it "includes all expected bits" do
      expect(subject.to_a).to match_array(enabled_bits.uniq)
    end
  end

  # In these tests create a large set of bitsets filled with random numbers.
  # Across all of the 500 bitsets, always enable the bits from the
  # `always_enabled` array so that they will appear in the intersection result.
  describe ".and_many" do
    let(:always_enabled) do
      Array.new(3) { (rand * 10_000).to_i }
    end

    let(:other_bits) do
      Array.new(500) { (rand * 10_000).to_i }
    end

    let(:sample_bitsets) do
      other_bits.map do |other_bit|
        described_class.new.tap do |bitset|
          bitset.add(other_bit)
          always_enabled.each { |bit| bitset.add(bit) }
        end
      end
    end

    subject { described_class.and_many(sample_bitsets) }

    it "returns a bitset" do
      expect(subject).to be_a(described_class)
    end

    it "includes all expected bits" do
      expect(subject.to_a).to match_array(always_enabled.uniq)
    end
  end
end
