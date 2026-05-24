## 1. Modify distance parsing in HighwayMilestoneFeature

- [x] 1.1 In `Parse()`, change `value->SetDistance(static_cast<uint32_t>(d))` to `value->SetDistance(static_cast<uint32_t>(d * 1000.0))` to convert OSM km to internal meters
- [x] 1.2 Update range check comment to note that the upper bound (numeric_limits<uint32_t>::max()) represents ~4.29M km after multiplication

## 2. Modify GetLabel output unit

- [x] 2.1 In `GetLabel()`, change `ss << NumberToString(distance, locale)` to `ss << NumberToString(distance / 1000, locale)`
- [x] 2.2 Change unit suffix from `"m"` to `"km"`

## 3. Modify DescriptionProcessor distance output

- [x] 3.1 In `HighwayMilestoneDescriptionProcessor::Process()`, change `std::to_string(value->GetDistance())` to `std::to_string(value->GetDistance() / 1000)` so DumpData shows kilometers

## 4. Add unit conversion tests

- [x] 4.1 Add test verifying `Parse()` multiplies km tag value by 1000 when storing meters
- [x] 4.2 Add test verifying `GetLabel()` divides internal meters by 1000 and outputs "km"
- [x] 4.3 Verify DescriptionProcessor outputs "km" suffix — confirmed by code inspection (line 799)