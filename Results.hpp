#include "Item.hpp"

class Results {
    string prev_link, next_link;
	string base_url;
	vector<Item> results;
	string current_page;
	string page;
	string title;
	
	GtkListStore *iconViewStore;
	map<string, GtkTreeIter> iters;
	
	string index; // save position of iconView
	public:
	
	string getIndex() {
		return index;
	}
	
	void setIndex(string i) {
		index = i;
	}
	
	void clearResultsAndModel(bool isPage) {
		if(!isPage) {
			getModelFromIconView();
			results.clear();
			iters.clear();
			gtk_list_store_clear(iconViewStore);
		}
	}
	
	void setTitle(string t) {
		title = t;
	}
	
	string getTitle() {
		return title;
	}
	
	void getResultsPage(string p) {
	    page = p;
	    parse_results();
	    parse_pager();         	
	}
	
	void setBaseUrl(string bu) {
	    base_url = bu;
	}
	
	string& getBaseUrl() {
		return base_url;
	}
	
	vector<Item> &getResults() {
	    return results;	
	}
	
	string& getPrevLink() {
		return prev_link;
	}
	
	string& getNextLink() {
		return next_link;
	}
	
	string getCurrentPage() {
		return current_page;
	}
	
	map<string, GtkTreeIter> &getIters() {
		return iters;
	}
	
	void copyToModel() {
		getModelFromIconView();
		gtk_list_store_clear(iconViewStore);
		iters.clear();
		for (unsigned i = 0; i < results.size(); i++) {
			appendToStore(results[i]);
		}
	}
	
	private:
	
	void getModelFromIconView() {
		iconViewStore = GTK_LIST_STORE(gtk_icon_view_get_model
				(GTK_ICON_VIEW(iconView))); 
	}
	
	void appendToStore(Item item) {
		static GtkTreeIter iter;
		static GdkPixbuf *pixbuf;
		string link = item.get_image_link();
		
		gtk_list_store_append(iconViewStore, &iter);
		// store iter by link in iters map
		iters[link] = iter;
		if(imagesCache.count(link) > 0) {
			pixbuf = imagesCache[link];
		}else {
			pixbuf = defaultPixbuf;
			// store iter by link in iters map
		    iters[link] = iter;
		}
		
        gtk_list_store_set(iconViewStore, &iter, IMAGE_COLUMN, pixbuf,
            TITLE_COLUMN, item.get_title().c_str(), -1);
	}
	
	//Parse search results
	void parse_results() {
		string domain(WDOMAIN);
		//results.clear();
	    string begin = "<div class=\"custom-poster\"";
		string end = "</a>";
		size_t div_begin = page.find(begin);
		size_t div_end = page.find(end, div_begin+1);
		while(div_end != string::npos && div_begin != string::npos) {
			size_t div_length = div_end - div_begin + end.length(); 
			string div = page.substr(div_begin, div_length);
			//cout << "Div: " << div << " Div END" << endl;
			//Find title
			size_t title_begin = div.find("/>");
			size_t title_end = div.find("</a>", title_begin + 1);
			if(title_begin != string::npos && title_end != string::npos) {
				size_t title_length = title_end - title_begin - 2;
				string title = div.substr(title_begin+2, title_length);
				size_t title_new_line = title.find("\n");
				if(title_new_line != string::npos) {//delete last char if last char is new line
				    title = div.substr(title_begin+2, title_new_line + 1);
					title.erase(title.size()-1);
				}
				//cout << "Title: " << title << endl;
				
				//Find href
				size_t href_begin = div.find("href=");
				size_t href_end = div.find(".html", href_begin + 1);
				if(href_begin != string::npos && href_end != string::npos) {
					size_t href_length = href_end - href_begin; 
					string href = div.substr(href_begin+6, href_length-1);
					//cout << "Href: " << href << endl;
					//Find id
					size_t id_begin = href.find(domain);
					size_t id_end = href.find("-", id_begin + domain.length());
					if(id_begin != string::npos && id_end != string::npos) {
						size_t id_length = id_end - id_begin - domain.length();
						string id_str = href.substr(id_begin + domain.length(), id_length);
						//cout << "Id: " << id_str << endl;
						//Find image
						size_t image_begin = div.find("src=");
						size_t image_end = div.find(".jpg", image_begin + 1);
						if(image_begin != string::npos && image_end != string::npos) {
							size_t image_length = image_end - image_begin;
							string image = div.substr(image_begin+5, image_length-1);
							
							Item item(title, id_str, href, image);
						    results.push_back(item);
						    appendToStore(item);
						}
					}
				}
			}
			
			div_begin = page.find(begin, div_end+1);
		    div_end = page.find(end, div_begin+1);
		}
	}
	
	void parse_anchor(string anchor) {
		size_t link_begin = anchor.find("=\"");
		size_t link_end = anchor.find(">", link_begin + 5);
		if(link_begin != string::npos && link_end != string::npos) {
			size_t link_length = link_end - link_begin;
			string link = anchor.substr(link_begin+2, link_length-3);
			string title = to_utf8(anchor.substr(link_end+1));
			
			if(title == "Вперед") {
				next_link = link;
			}
			
			if(title == "Назад") {
				prev_link = link;
			}
		}
	}
	
	void parse_pager() {
		prev_link = next_link = current_page = "";
		size_t pager_begin = page.find("class=\"navigation\"");
		size_t pager_end = page.find("</div>", pager_begin+1);
		if(pager_end != string::npos && pager_begin != string::npos) {
			size_t pager_length = pager_end - pager_begin;
			string pager = page.substr(pager_begin+2, pager_length-2);
	        
	        // Find spans
	        string begin_span = "<span>";
	        string end_span = "</span>";
	        size_t span_begin = pager.find(begin_span);
	        size_t span_end = pager.find(end_span);
	        int count_span = 0;
	        string spans[2];
	        int pages[2];
	        while(span_end != string::npos && span_begin != string::npos) {
				size_t span_length = span_end - span_begin;
				spans[count_span] = pager.substr(span_begin+6, span_length-6);
				pages[count_span] = atoi(spans[count_span].c_str());
				count_span++;
				span_begin = pager.find(begin_span, span_end);
	            span_end = pager.find(end_span, span_begin);
			}
			current_page = "";
			if(count_span == 1) {
				current_page = spans[0];
			}else {
				if(pages[0] != 0) {
					current_page = spans[0];
				}
				if(pages[1] != 0) {
					current_page = spans[1];
				}
			}
	        
	        //Find menu pager anchors
	        // <a href="http://google.com.ua">Google</a>
	        string begin = "<a href";
		    string end = "</a>";
	        size_t anchor_begin = pager.find(begin);
	        size_t anchor_end = pager.find(end, anchor_begin+1);
	        string anchor;
	        while(anchor_end != string::npos && anchor_begin != string::npos) {
				size_t anchor_length = anchor_end - anchor_begin;
				anchor = pager.substr(anchor_begin, anchor_length);
				
				parse_anchor(anchor);
				
			    anchor_begin = pager.find(begin, anchor_end);
	            anchor_end = pager.find(end, anchor_begin);
			}
			
			//Find search pager anchors
		    begin = "onclick";
	        anchor_begin = pager.find(begin);
	        anchor_end = pager.find(end, anchor_begin+1);
	        while(anchor_end != string::npos && anchor_begin != string::npos) {
				size_t anchor_length = anchor_end - anchor_begin;
				anchor = pager.substr(anchor_begin, anchor_length);
				
				size_t page_begin = anchor.find("(");
				size_t page_end = anchor.find(")", page_begin+1);
				if(page_begin != string::npos && page_end != string::npos) {
					size_t page_length = page_end - page_begin;
					string page_num = anchor.substr(page_begin+1, page_length-1);
					
					size_t title_begin = anchor.find(">");
					string title = to_utf8(anchor.substr(title_begin+1));
					
					if(title == "Вперед") {
						next_link = base_url + "&search_start=" + page_num;
					}
					
					if(title == "Назад") {
						prev_link = base_url + "&search_start=" + page_num;
					}
				}
				
			    anchor_begin = pager.find(begin, anchor_end);
	            anchor_end = pager.find(end, anchor_begin);
			}
		}
	}

};
