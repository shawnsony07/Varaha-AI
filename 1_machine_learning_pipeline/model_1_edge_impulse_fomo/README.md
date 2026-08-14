# Model 1 — Edge Impulse FOMO (Tier 1 Baseline)

## Architecture
Faster Objects More Objects (FOMO) — a lightweight centroid-detection model designed for microcontrollers. Trained via Edge Impulse on the Varaha hog dataset.

## Purpose
Initial prototype to validate the feasibility of on-device pig detection. FOMO outputs object centroids rather than bounding boxes, making it extremely fast but less precise.

## Key Results
See model.4_fomo_8.json for evaluation metrics and confusion matrix output.

## Limitations
- No bounding box output (centroid only)
- Lower precision in cluttered outdoor scenes
- Superseded by Edge Impulse YOLO (Tier 2)
