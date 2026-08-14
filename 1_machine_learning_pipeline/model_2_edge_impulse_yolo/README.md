# Model 2 — Edge Impulse YOLO (Tier 2 Baseline)

## Architecture
Standard bounding-box YOLO model trained via Edge Impulse on the Varaha hog dataset.

## Purpose
Upgrade from FOMO to full bounding-box detection. Validated strong mAP before moving to hardware-optimized Swift-YOLO.

## Key Results
See model.4_yolo.json for COCO evaluation metrics.
- **mAP@50: 77.4%**

## Limitations
- Not optimized for Arm Ethos-U55 NPU
- Larger model footprint vs Swift-YOLO
- Superseded by Swift-YOLO SSCMA (Tier 3)
