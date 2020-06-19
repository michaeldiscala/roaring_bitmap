require "roaring_bitmap_ext"

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
end
