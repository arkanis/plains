#include <stdio.h>
#include <fitz.h>

void main(){
	char *path = "JacobssonCramerHRI2011lbr.pdf";
	
	fz_context *ctx = fz_new_context(NULL, NULL, FZ_STORE_UNLIMITED);
	fz_document *doc = fz_open_document(ctx, path);
	
	int page_count = fz_count_pages(doc);
	printf("%d pages\n", page_count);
	
	for(size_t page_idx = 0; page_idx < page_count; page_idx++){
		printf("page %zu:\n", page_idx);
		fz_page *page = fz_load_page(doc, page_idx);
		
		fz_rect rect = fz_bound_page(doc, page);
		fz_bbox bbox = fz_round_rect(rect);
		printf("  rect: x %f, y %f, w %f, h %f\n", rect.x0, rect.y0, rect.x1 - rect.x0, rect.y1 - rect.y0);
		
		fz_pixmap *pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb, bbox);
		fz_clear_pixmap_with_value(ctx, pix, 0xff);
		
		fz_device *dev = fz_new_draw_device(ctx, pix);
		fz_run_page(doc, page, dev, fz_identity, NULL);
		fz_free_device(dev);
		
		printf("  rendered, %d components, data: %p\n", fz_pixmap_components(ctx, pix), fz_pixmap_samples(ctx, pix));
		
		fz_free_page(doc, page);
	}
	
	fz_close_document(doc);
	fz_free_context(ctx);
}