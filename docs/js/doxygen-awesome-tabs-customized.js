// SPDX-License-Identifier: MIT
/**

Based on

Doxygen Awesome
https://github.com/jothepro/doxygen-awesome-css

Copyright (c) 2026 articy software GmbH & Co.

*/

class DoxygenAwesomeTabs {

    static init() {
        window.addEventListener("load", () => {
            const allTabbedInstances = document.querySelectorAll(".tabbed:not(:empty)");
        
            const savedGroup = localStorage.getItem("code-examples-language");

            allTabbedInstances.forEach((tabbed, tabbedIndex) => {
                let tabLinkList = [];
                let tabToSelectInitially = null;

                tabbed.querySelectorAll(":scope > ul > li").forEach((tab, tabIndex) => {
                    tab.id = "tab_" + tabbedIndex + "_" + tabIndex;
                    let header = tab.querySelector(".tab-title");
                    
                    let group = tab.getAttribute("data-group");

                    let tabLink = document.createElement("button");
                    tabLink.classList.add("tab-button");
                    tabLink.appendChild(header);
                    header.title = header.textContent;

                    const selectTab = (isManualClick = true) => {
                        tabbed.querySelectorAll(":scope > ul > li").forEach((t) => t.classList.remove("selected"));
                        tabLinkList.forEach((link) => link.classList.remove("active"));

                        tab.classList.add("selected");
                        tabLink.classList.add("active");

                        if (isManualClick && group) {
                            localStorage.setItem("code-examples-language", group);

                            window.dispatchEvent(new CustomEvent("tabGroupChanged", { 
                                detail: { group: group } 
                            }));
                        }
                    };

                    tabLink.addEventListener("click", () => selectTab(true));
                    
                    window.addEventListener("tabGroupChanged", (e) => {
                        if (group && e.detail.group === group) {
                            selectTab(false);
                        }
                    });

                    tabLinkList.push(tabLink);

                    if (group && group === savedGroup) {
                        tabToSelectInitially = { tab, tabLink };
                    }
                    if (!tabToSelectInitially && tabIndex === 0) {
                        tabToSelectInitially = { tab, tabLink };
                    }
                });

                if (tabToSelectInitially) {
                    tabToSelectInitially.tab.classList.add("selected");
                    tabToSelectInitially.tabLink.classList.add("active");
                }

                let tabsOverview = document.createElement("div")
                tabsOverview.classList.add("tabs-overview")
                let tabsOverviewContainer = document.createElement("div")
                tabsOverviewContainer.classList.add("tabs-overview-container")
                tabLinkList.forEach((tabLink) => {
                    tabsOverview.appendChild(tabLink)
                })
                tabsOverviewContainer.appendChild(tabsOverview)
                tabbed.before(tabsOverviewContainer)
            })
        })
        
    }
}